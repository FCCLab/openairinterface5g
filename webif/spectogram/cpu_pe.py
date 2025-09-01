#!/usr/bin/env python3
"""
Detect Intel P-cores vs E-cores per logical CPU using CPUID only.

Mechanism:
  - Pin to each logical CPU with os.sched_setaffinity
  - Execute CPUID leaf 0x1A (Hybrid Information Enumeration)
    * EAX[31:24] = core type:
        0x40 -> P-core (Performance)
        0x20 -> E-core (Efficiency/Atom)
        0x00 -> Not hybrid / unknown on that core
        other values -> vendor/reserved/unknown (printed)

Notes:
  - Requires: pip install cpuid
  - Linux only (uses sched_setaffinity). For Windows, switch to SetThreadAffinityMask.
  - If the CPU/VM doesn't support hybrid features, you'll see "Unknown(0x..)" or "Not supported".
"""

import os
import sys
from typing import Optional, Tuple

try:
    import cpuid  # PyPI: cpuid
except Exception as e:
    print(f"ERROR: Unable to import 'cpuid' package: {e}", file=sys.stderr)
    sys.exit(1)

# Some builds accept cpuid.cpuid(leaf) only; others accept (leaf, subleaf).
def _cpuid(leaf: int, subleaf: int = 0) -> Tuple[int, int, int, int]:
    try:
        return cpuid.cpuid(leaf, subleaf)  # try 2-arg signature first
    except TypeError:
        return cpuid.cpuid(leaf)           # fallback to 1-arg signature

def _hybrid_supported() -> bool:
    """Check if CPUID indicates Hybrid feature is present (CPUID.7.0:EDX[15])."""
    try:
        # CPUID.(EAX=7, ECX=0)
        eax, ebx, ecx, edx = _cpuid(0x7, 0)
        # EDX bit 15 = Hybrid
        return bool((edx >> 15) & 1)
    except Exception:
        return False

def _max_basic_leaf() -> int:
    """Return the max basic CPUID leaf from CPUID.(EAX=0)."""
    try:
        eax, ebx, ecx, edx = _cpuid(0x0, 0)
        return eax
    except Exception:
        return 0

def core_type_current_cpu() -> Optional[str]:
    """
    Call CPUID 0x1A on the *current* CPU (whatever the thread is pinned to).
    Returns: 'P-core' | 'E-core' | 'Unknown(0x..)' | 'Not supported' | None (on failure)
    """
    try:
        if _max_basic_leaf() < 0x1A:
            return "Not supported"
        eax, ebx, ecx, edx = _cpuid(0x1A, 0)
    except Exception:
        return None

    t = (eax >> 24) & 0xFF  # EAX[31:24]
    if t == 0x40:
        return "P-core"
    elif t == 0x20:
        return "E-core"
    elif t == 0x00:
        # Leaf exists but not a hybrid core (could be non-hybrid CPU)
        return "Unknown(0x00)"
    else:
        return f"Unknown(0x{t:02x})"

def classify_all_logical_cpus():
    ncpu = os.cpu_count() or 1
    rows = []

    # Save current affinity to restore later
    try:
        old_aff = os.sched_getaffinity(0)
    except AttributeError:
        print("ERROR: os.sched_setaffinity not available on this platform.", file=sys.stderr)
        sys.exit(1)

    try:
        for cpu in range(ncpu):
            try:
                os.sched_setaffinity(0, {cpu})
            except PermissionError:
                # Usually needs CAP_SYS_NICE or not confined by container cpuset
                rows.append((cpu, "Affinity denied"))
                continue

            ctype = core_type_current_cpu()
            rows.append((cpu, ctype if ctype is not None else "Error"))
    finally:
        # Restore original affinity no matter what
        try:
            os.sched_setaffinity(0, old_aff)
        except Exception:
            pass

    return rows

def get_p_e_cores():
    """
    Common function to return two lists: P-cores and E-cores.
    
    Returns:
        tuple: (p_cores_list, e_cores_list) where each is a list of CPU core numbers
        
    Example:
        p_cores, e_cores = get_p_e_cores()
        print(f"P-cores: {p_cores}")  # e.g., [8, 9, 10, 11, 12, 13, 14, 15]
        print(f"E-cores: {e_cores}")  # e.g., [0, 1, 2, 3, 4, 5, 6, 7]
    """
    try:
        # Get all core classifications
        core_classifications = classify_all_logical_cpus()
        
        # Separate into P-cores and E-cores
        p_cores = []
        e_cores = []
        
        for cpu, core_type in core_classifications:
            if core_type == "P-core":
                p_cores.append(cpu)
            elif core_type == "E-core":
                e_cores.append(cpu)
            else:
                # Unknown, error, or affinity denied - treat as P-core for safety
                p_cores.append(cpu)
        
        return sorted(p_cores), sorted(e_cores)
        
    except Exception as e:
        print(f"ERROR: Failed to get P/E cores: {e}", file=sys.stderr)
        # Fallback: return all cores as P-cores
        ncpu = os.cpu_count() or 1
        return list(range(ncpu)), []

def test_get_p_e_cores():
    """Test function for the get_p_e_cores() function."""
    print("Testing get_p_e_cores() function...")
    try:
        p_cores, e_cores = get_p_e_cores()
        print(f"✓ P-cores: {p_cores}")
        print(f"✓ E-cores: {e_cores}")
        print(f"✓ Total cores: {len(p_cores) + len(e_cores)}")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

if __name__ == "__main__":
    # Test the new common function
    test_get_p_e_cores()
