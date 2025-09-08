#!/usr/bin/env python3
"""
Simulated UE Process - Python Implementation
This process simulates a UE (User Equipment) and provides gRPC services
for control and data acquisition.
"""

import grpc
import time
import random
import math
import json
import threading
import signal
import sys
import logging
from concurrent import futures
from typing import List, Dict, Any
import argparse

# Import generated protobuf classes (we'll generate these)

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


try:
    from proto import ue_service_pb2
    from proto import ue_service_pb2_grpc
except ImportError:
    try:
        import ue_service_pb2
        import ue_service_pb2_grpc
    except ImportError as e:
        print(f"Error importing protobuf files: {e}")
        print("Run: python -m grpc_tools.protoc --python_out=. --grpc_python_out=. proto/ue_service.proto")
        sys.exit(1)

# Configure logging to file
import os
from datetime import datetime

# Create logs directory if it doesn't exist
log_dir = 'logs'
if not os.path.exists(log_dir):
    os.makedirs(log_dir)

# Create log filename with timestamp
log_filename = f"{log_dir}/simulated_ue_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(log_filename),
        logging.StreamHandler(sys.stdout)  # Also log to console
    ]
)
logger = logging.getLogger('SimulatedUE')

# Log startup information
logger.info(f"Simulated UE Process starting - Log file: {log_filename}")

class SimulatedUEProcess:
    """Simulated UE Process that provides gRPC services"""
    
    def __init__(self, port: int = 50051):
        self.port = port
        self.is_running = False
        self.process_id = None
        self.start_time = None
        self.config = {}
        self.connection_state = 'disconnected'
        self.detected_cells = []
        self.signal_quality = {
            'rsrp': -120.0,
            'rsrq': -20.0,
            'sinr': 0.0
        }
        self.cell_info = None
        self.logs = []
        self.metrics = {
            'total_uptime': 0,
            'cell_scans': 0,
            'connection_attempts': 0,
            'successful_connections': 0,
            'errors': 0
        }
        
        # Scanning state
        self.is_scanning = False
        self.scan_thread = None
        self.scan_stop_event = threading.Event()
        
        # Threading
        self.cell_scan_thread = None
        self.signal_update_thread = None
        self.stop_event = threading.Event()
        
        # gRPC server
        self.server = None
        
    def start(self, config: Dict[str, Any]) -> Dict[str, Any]:
        """Start the simulated UE process"""
        if self.is_running:
            raise Exception("Simulated UE process is already running")
        
        self.config = config
        self.process_id = f"sim_ue_{int(time.time() * 1000)}"
        self.start_time = time.time()
        self.is_running = True
        self.connection_state = 'initializing'
        
        self.log("Starting simulated UE process", "info")
        self.log(f"Process ID: {self.process_id}", "info")
        self.log(f"Frequency: {config.get('frequency', 'N/A')} Hz", "info")
        self.log(f"IMSI: {config.get('imsi', 'N/A')}", "info")
        
        # Start background threads
        self.start_background_threads()
        
        # Start gRPC server
        self.start_grpc_server()
        
        return {
            'success': True,
            'process_id': self.process_id,
            'message': 'Simulated UE process started successfully',
            'config': self.config,
            'grpc_port': self.port
        }
    
    def stop(self) -> Dict[str, Any]:
        """Stop the simulated UE process"""
        if not self.is_running:
            raise Exception("Simulated UE process is not running")
        
        self.log("Stopping simulated UE process", "info")
        
        # Stop background threads
        self.stop_event.set()
        
        # Stop gRPC server
        if self.server:
            self.server.stop(0)
        
        self.is_running = False
        self.connection_state = 'disconnected'
        self.detected_cells = []
        self.cell_info = None
        
        uptime = time.time() - self.start_time
        self.metrics['total_uptime'] += uptime
        
        self.log(f"Process stopped. Total uptime: {uptime:.1f}s", "info")
        
        return {
            'success': True,
            'message': 'Simulated UE process stopped successfully',
            'uptime': uptime
        }
    
    def start_background_threads(self):
        """Start background simulation threads"""
        # Cell scanning thread
        self.cell_scan_thread = threading.Thread(target=self.cell_scanning_loop, daemon=True)
        self.cell_scan_thread.start()
        
        # Signal quality update thread
        self.signal_update_thread = threading.Thread(target=self.signal_quality_loop, daemon=True)
        self.signal_update_thread.start()
        
        # Connection simulation thread
        connection_thread = threading.Thread(target=self.simulate_connection, daemon=True)
        connection_thread.start()
    
    def cell_scanning_loop(self):
        """Background cell scanning simulation - only runs when explicitly started"""
        while not self.stop_event.is_set():
            if self.is_running and self.is_scanning:
                self.perform_cell_scan()
            time.sleep(5)  # Check every 5 seconds
    
    def signal_quality_loop(self):
        """Background signal quality update simulation"""
        while not self.stop_event.is_set():
            if self.is_running:
                self.update_signal_quality()
            time.sleep(2)  # Update every 2 seconds
    
    def perform_cell_scan(self):
        """Perform a cell scan"""
        self.metrics['cell_scans'] += 1
        self.log(f"Performing cell scan (scan #{self.metrics['cell_scans']})", "debug")
        
        # Generate exactly 3 cells with specific PCI values
        new_cells = self.generate_initial_cells()
        
        # Sort by signal strength
        new_cells.sort(key=lambda x: x['ss_rsrp'], reverse=True)
        
        self.detected_cells = new_cells
        self.log(f"Found {len(new_cells)} cells with PCI values: {[cell['pci'] for cell in new_cells]}", "info")
    
    def generate_random_cell(self, cell_id: int) -> Dict[str, Any]:
        """Generate a random cell"""
        pss = random.randint(0, 2)
        sss = random.randint(0, 335)
        pci = 3 * sss + pss
        
        return {
            'id': cell_id,
            'pci': pci,
            'pss': {'detected': True, 'value': pss},
            'sss': {'detected': True, 'value': sss},
            'ss_rsrp': -60 - random.random() * 50,  # -60 to -110 dBm
            'ss_rsrq': -10 - random.random() * 15,  # -10 to -25 dB
            'ss_sinr': random.random() * 20,        # 0 to 20 dB
            'pbch': 'Decoded' if random.random() > 0.3 else 'Not decoded',
            'sib1': 'Detected' if random.random() > 0.4 else 'Not detected',
            'mib': self.generate_random_mib() if random.random() > 0.3 else None,
            'sib1_info': self.generate_random_sib1() if random.random() > 0.4 else None,
            'distance': random.randint(50, 2050),  # 50-2050m
            'frequency': self.config.get('frequency', 3425010000) + random.randint(-500000, 500000)
        }
    
    def generate_random_mib(self) -> Dict[str, Any]:
        """Generate random MIB"""
        return {
            'system_frame_number': random.randint(0, 1023),
            'subcarrier_spacing_common': random.randint(1, 3),
            'ssb_subcarrier_offset': random.randint(0, 11),
            'dmrs_type_a_position': random.randint(0, 3),
            'pdcch_config_sib1': random.randint(0, 15),
            'cell_barred': random.random() > 0.8,
            'intra_freq_reselection': random.random() > 0.2,
            'spare': 0
        }
    
    def generate_random_sib1(self) -> Dict[str, Any]:
        """Generate random SIB1"""
        return {
            'cell_identity': random.randint(0, 99999),
            'plmn_identity': f"{random.randint(0, 999):03d}-{random.randint(0, 99):02d}",
            'tracking_area_code': random.randint(0, 999),
            'cell_reserved_for_operator_use': random.random() > 0.7,
            'cell_selection_info': {
                'q_rx_lev_min': -70 - random.randint(0, 20),
                'q_qual_min': -20 - random.randint(0, 10)
            },
            'freq_band_indicator': random.randint(1, 100),
            'scheduling_info_list': [
                {'si_periodicity': 'ms80', 'sib_mapping_info': [1, 2, 3]}
            ],
            'si_window_length': random.choice(['ms20', 'ms40', 'ms80', 'ms160']),
            'system_info_value_tag': random.randint(0, 31),
            'late_non_critical_extension': random.random() > 0.5,
            'non_critical_extension': random.random() > 0.5
        }
    
    def update_signal_quality(self):
        """Update signal quality"""
        variation = (random.random() - 0.5) * 5  # ±2.5 dB variation
        
        self.signal_quality['rsrp'] = max(-140, min(-50, self.signal_quality['rsrp'] + variation))
        self.signal_quality['rsrq'] = max(-30, min(-5, self.signal_quality['rsrq'] + variation * 0.5))
        self.signal_quality['sinr'] = max(0, min(25, self.signal_quality['sinr'] + variation * 0.3))
    
    def simulate_connection(self):
        """Simulate connection process"""
        time.sleep(3)  # Initial delay
        
        self.metrics['connection_attempts'] += 1
        self.log("Attempting to connect to network...", "info")
        
        # Simulate connection delay
        time.sleep(3 + random.random() * 2)
        
        if random.random() > 0.1:  # 90% success rate
            self.connection_state = 'connected'
            self.metrics['successful_connections'] += 1
            
            self.cell_info = {
                'cell_id': self.detected_cells[0]['pci'] if self.detected_cells else 123,
                'frequency': self.config.get('frequency', 3425010000),
                'bandwidth': self.config.get('bandwidth', 100),
                'plmn': '001-01',
                'tac': random.randint(0, 999)
            }
            
            self.log(f"Connected to cell PCI {self.cell_info['cell_id']}", "info")
            self.log(f"PLMN: {self.cell_info['plmn']}", "info")
        else:
            self.connection_state = 'failed'
            self.metrics['errors'] += 1
            self.log("Connection failed, retrying...", "warn")
    
    def start_grpc_server(self):
        """Start the gRPC server"""
        self.server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        
        # Add services
        ue_service_pb2_grpc.add_UEControlServiceServicer_to_server(
            UEControlServiceImpl(self), self.server
        )
        ue_service_pb2_grpc.add_UEDataServiceServicer_to_server(
            UEDataServiceImpl(self), self.server
        )
        
        # Start server
        listen_addr = f'[::]:{self.port}'
        self.server.add_insecure_port(listen_addr)
        self.server.start()
        
        self.log(f"gRPC server started on port {self.port}", "info")
    
    def get_status(self) -> Dict[str, Any]:
        """Get current status"""
        return {
            'is_running': self.is_running,
            'process_id': self.process_id,
            'start_time': self.start_time,
            'connection_state': self.connection_state,
            'signal_quality': self.signal_quality,
            'cell_info': self.cell_info,
            'detected_cells': self.detected_cells,
            'metrics': self.metrics,
            'config': self.config,
            'uptime': time.time() - self.start_time if self.is_running else 0,
            'is_scanning': self.is_scanning
        }
    
    def start_scanning(self) -> Dict[str, Any]:
        """Start cell scanning procedure"""
        if not self.is_running:
            raise Exception("Simulated UE process is not running")
        
        if self.is_scanning:
            return {
                'success': True,
                'message': 'Cell scan is already running'
            }
        
        self.is_scanning = True
        self.scan_stop_event.clear()
        
        self.log("Starting cell scanning", "info")
        
        # Start scanning thread
        self.scan_thread = threading.Thread(target=self.cell_scanning_procedure, daemon=True)
        self.scan_thread.start()
        
        return {
            'success': True,
            'message': 'Cell scanning started successfully'
        }
    
    def stop_scanning(self) -> Dict[str, Any]:
        """Stop cell scanning procedure"""
        if not self.is_scanning:
            raise Exception("No scanning in progress")
        
        self.log("Stopping cell scanning", "info")
        
        # Signal stop to scanning thread
        self.scan_stop_event.set()
        
        # Wait for thread to finish (with timeout)
        if self.scan_thread and self.scan_thread.is_alive():
            self.scan_thread.join(timeout=2.0)
        
        self.is_scanning = False
        
        return {
            'success': True,
            'message': 'Cell scanning stopped successfully'
        }
    
    def cell_scanning_procedure(self):
        """Cell scanning procedure that generates 3 cells with varying signal quality"""
        self.log("Cell scanning procedure started", "info")
        
        # Generate 3 initial cells
        cells = self.generate_initial_cells()
        self.detected_cells = cells
        
        self.log(f"Initial scan complete: {len(cells)} cells detected", "info")
        
        # Update cells periodically with varying signal quality
        update_count = 0
        while not self.scan_stop_event.is_set() and self.is_scanning:
            try:
                # Update signal quality for all cells
                for cell in self.detected_cells:
                    self.update_cell_signal_quality(cell)
                
                update_count += 1
                self.log(f"Cell signal quality updated (update #{update_count})", "debug")
                
                # Wait 2 seconds before next update
                if self.scan_stop_event.wait(2.0):
                    break
                    
            except Exception as e:
                self.log(f"Error in cell scanning procedure: {str(e)}", "error")
                break
        
        self.log("Cell scanning procedure ended", "info")
    
    def generate_initial_cells(self) -> List[Dict[str, Any]]:
        """Generate exactly 3 cells with specific PCI values: 1, 500, 1000"""
        cells = []
        
        # Define the 3 specific PCI values
        pci_values = [1, 500, 1000]
        
        for i, pci in enumerate(pci_values):
            # Calculate PSS and SSS from PCI
            # PCI = 3 * SSS + PSS
            sss = pci // 3
            pss = pci % 3
            
            # Generate realistic signal quality with some variation
            base_rsrp = -85 + (i * 5)  # Different base levels for each cell
            ss_rsrp = base_rsrp + random.uniform(-5, 5)  # Add some variation
            ss_rsrq = ss_rsrp - random.uniform(3, 8)     # RSRQ is typically lower
            ss_sinr = ss_rsrp + random.uniform(5, 15)    # SINR is typically higher
            
            cell = {
                'pci': pci,
                'pss': pss,
                'sss': sss,
                'ss_rsrp': round(ss_rsrp, 1),
                'ss_rsrq': round(ss_rsrq, 1),
                'ss_sinr': round(ss_sinr, 1),
                'pbch_decoded': random.choice([True, False]),
                'mib': self.generate_mib_info() if random.choice([True, False]) else None,
                'sib1_detected': random.choice([True, False]),
                'sib1': self.generate_sib1_info() if random.choice([True, False]) else None,
                'cell_id': f"cell_{i+1}",
                'frequency': self.config.get('frequency', 3425010000),
                'timestamp': time.time()
            }
            cells.append(cell)
        
        return cells
    
    def update_cell_signal_quality(self, cell: Dict[str, Any]):
        """Update signal quality for a cell with realistic variations over time"""
        # Create more realistic signal quality changes
        # Add time-based variations (simulate fading, interference, etc.)
        time_factor = time.time() % 60  # 60-second cycle
        
        # Different variation patterns for different PCI values
        pci = cell['pci']
        if pci == 1:
            # Cell 1: Strong signal with moderate variations
            base_variation = 1.5
            time_variation = 2 * math.sin(time_factor * 0.1)  # Slow variation
        elif pci == 500:
            # Cell 500: Medium signal with more variations
            base_variation = 2.5
            time_variation = 3 * math.sin(time_factor * 0.15)  # Medium variation
        else:  # pci == 1000
            # Cell 1000: Weaker signal with high variations
            base_variation = 3.5
            time_variation = 4 * math.sin(time_factor * 0.2)  # Fast variation
        
        # Add random noise and time-based variations
        variation_rsrp = random.uniform(-base_variation, base_variation) + time_variation
        variation_rsrq = random.uniform(-base_variation/2, base_variation/2) + time_variation/2
        variation_sinr = random.uniform(-base_variation/2, base_variation/2) + time_variation/2
        
        # Update signal quality with realistic bounds
        cell['ss_rsrp'] = round(max(-140, min(-50, cell['ss_rsrp'] + variation_rsrp)), 1)
        cell['ss_rsrq'] = round(max(-30, min(-3, cell['ss_rsrq'] + variation_rsrq)), 1)
        cell['ss_sinr'] = round(max(-10, min(30, cell['ss_sinr'] + variation_sinr)), 1)
        cell['timestamp'] = time.time()
        
        # Occasionally update PBCH/SIB1 status
        if random.random() < 0.1:  # 10% chance
            cell['pbch_decoded'] = random.choice([True, False])
            if cell['pbch_decoded'] and not cell['mib']:
                cell['mib'] = self.generate_mib_info()
        
        if random.random() < 0.05:  # 5% chance
            cell['sib1_detected'] = random.choice([True, False])
            if cell['sib1_detected'] and not cell['sib1']:
                cell['sib1'] = self.generate_sib1_info()
    
    def generate_mib_info(self) -> Dict[str, str]:
        """Generate realistic MIB information"""
        return {
            'system_frame_number': str(random.randint(0, 1023)),
            'subcarrier_spacing': random.choice(['15kHz', '30kHz', '60kHz', '120kHz']),
            'ssb_subcarrier_offset': str(random.randint(0, 11)),
            'dmrs_type_a_position': random.choice(['pos2', 'pos3']),
            'pdcch_config_sib1': f"0x{random.randint(0, 255):02x}",
            'cell_barred': random.choice(['barred', 'not_barred']),
            'intra_freq_reselection': random.choice(['allowed', 'not_allowed']),
            'spare': str(random.randint(0, 7))
        }
    
    def generate_sib1_info(self) -> Dict[str, str]:
        """Generate realistic SIB1 information"""
        return {
            'cell_access_related_info': f"0x{random.randint(0, 65535):04x}",
            'cell_selection_info': f"0x{random.randint(0, 65535):04x}",
            'p_max': str(random.randint(-30, 23)),
            'frequency_band_list': f"n{random.randint(1, 100)}",
            'scs_specific_carrier_list': f"0x{random.randint(0, 255):02x}",
            'tdd_ul_dl_configuration_common': str(random.randint(0, 255)),
            'ssb_positions_in_burst': f"0x{random.randint(0, 255):02x}",
            'ssb_periodicity_serving_cell': random.choice(['5ms', '10ms', '20ms', '40ms', '80ms', '160ms']),
            'dmrs_type_a_position': random.choice(['pos2', 'pos3']),
            'pdcch_config_sib1': f"0x{random.randint(0, 255):02x}"
        }
    
    def get_logs(self, lines: int = 100) -> List[Dict[str, Any]]:
        """Get recent logs"""
        return self.logs[-lines:]
    
    def log(self, message: str, level: str = "info"):
        """Add log entry"""
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
        log_entry = {
            'timestamp': timestamp,
            'level': level,
            'message': message,
            'process_id': self.process_id
        }
        
        self.logs.append(log_entry)
        
        # Keep only last 1000 logs
        if len(self.logs) > 1000:
            self.logs = self.logs[-1000:]
        
        # Also log to system logger
        getattr(logger, level)(f"[SimulatedUE] {message}")


class UEControlServiceImpl(ue_service_pb2_grpc.UEControlServiceServicer):
    """gRPC UE Control Service Implementation"""
    
    def __init__(self, ue_process: SimulatedUEProcess):
        self.ue_process = ue_process
    
    def StartUE(self, request, context):
        """Start UE process"""
        self.ue_process.log(f"gRPC StartUE called with frequency: {request.frequency}", "info")
        
        return ue_service_pb2.StartUEResponse(
            success=True,
            message="UE started via gRPC",
            process_id=self.ue_process.process_id or "unknown"
        )
    
    def StopUE(self, request, context):
        """Stop UE process"""
        self.ue_process.log("gRPC StopUE called", "info")
        
        return ue_service_pb2.StopUEResponse(
            success=True,
            message="UE stopped via gRPC"
        )
    
    def ConfigureUE(self, request, context):
        """Configure UE"""
        self.ue_process.log("gRPC ConfigureUE called", "info")
        
        return ue_service_pb2.ConfigureUEResponse(
            success=True,
            message="UE configuration updated via gRPC"
        )
    
    def StartScanning(self, request, context):
        """Start cell scanning procedure"""
        self.ue_process.log("gRPC StartScanning called", "info")
        
        try:
            result = self.ue_process.start_scanning()
            
            return ue_service_pb2.StartScanningResponse(
                success=result['success'],
                message=result['message']
            )
        except Exception as e:
            self.ue_process.log(f"Error starting scanning: {str(e)}", "error")
            
            return ue_service_pb2.StartScanningResponse(
                success=False,
                message=f"Failed to start scanning: {str(e)}"
            )
    
    def StopScanning(self, request, context):
        """Stop cell scanning procedure"""
        self.ue_process.log("gRPC StopScanning called", "info")
        
        try:
            result = self.ue_process.stop_scanning()
            
            return ue_service_pb2.StopScanningResponse(
                success=result['success'],
                message=result['message']
            )
        except Exception as e:
            self.ue_process.log(f"Error stopping scanning: {str(e)}", "error")
            
            return ue_service_pb2.StopScanningResponse(
                success=False,
                message=f"Failed to stop scanning: {str(e)}"
            )


class UEDataServiceImpl(ue_service_pb2_grpc.UEDataServiceServicer):
    """gRPC UE Data Service Implementation"""
    
    def __init__(self, ue_process: SimulatedUEProcess):
        self.ue_process = ue_process
    
    def GetDetectedCells(self, request, context):
        """Get detected cells"""
        try:
            self.ue_process.log(f"GetDetectedCells called, found {len(self.ue_process.detected_cells)} cells", "info")
            cells = []
            for cell in self.ue_process.detected_cells:
                mib = None
                if cell.get('mib') and isinstance(cell['mib'], dict):
                    try:
                        mib = ue_service_pb2.MIBInfo(
                            system_frame_number=str(cell['mib'].get('system_frame_number', '0')),
                            subcarrier_spacing=str(cell['mib'].get('subcarrier_spacing_common', '15kHz')),
                            ssb_subcarrier_offset=str(cell['mib'].get('ssb_subcarrier_offset', '0')),
                            dmrs_type_a_position=str(cell['mib'].get('dmrs_type_a_position', 'pos2')),
                            pdcch_config_sib1=str(cell['mib'].get('pdcch_config_sib1', '0x0')),
                            cell_barred=str(cell['mib'].get('cell_barred', 'not_barred')),
                            intra_freq_reselection=str(cell['mib'].get('intra_freq_reselection', 'allowed')),
                            spare=str(cell['mib'].get('spare', '0'))
                        )
                    except Exception as e:
                        self.ue_process.log(f"Error creating MIB for cell {cell.get('id', 'unknown')}: {str(e)}", "error")
                        mib = None
                
                sib1 = None
                if cell.get('sib1_info') and isinstance(cell['sib1_info'], dict):
                    try:
                        sib1 = ue_service_pb2.SIB1Info(
                            cell_access_related_info=str(cell['sib1_info'].get('cell_identity', '0x0')),
                            cell_selection_info=str(cell['sib1_info'].get('cell_selection_info', '0x0')),
                            p_max=str(cell['sib1_info'].get('p_max', '23')),
                            frequency_band_list=str(cell['sib1_info'].get('freq_band_indicator', 'n1')),
                            scs_specific_carrier_list=str(cell['sib1_info'].get('scs_specific_carrier_list', '0x0')),
                            tdd_ul_dl_configuration_common=str(cell['sib1_info'].get('tdd_ul_dl_configuration_common', '0')),
                            ssb_positions_in_burst=str(cell['sib1_info'].get('ssb_positions_in_burst', '0x0')),
                            ssb_periodicity_serving_cell=str(cell['sib1_info'].get('ssb_periodicity_serving_cell', '20ms')),
                            dmrs_type_a_position=str(cell['sib1_info'].get('dmrs_type_a_position', 'pos2')),
                            pdcch_config_sib1=str(cell['sib1_info'].get('pdcch_config_sib1', '0x0'))
                        )
                    except Exception as e:
                        self.ue_process.log(f"Error creating SIB1 for cell {cell.get('id', 'unknown')}: {str(e)}", "error")
                        sib1 = None
                
                # Extract PSS and SSS values
                pss_value = 0
                if isinstance(cell.get('pss'), dict):
                    pss_value = cell['pss'].get('value', 0)
                else:
                    pss_value = cell.get('pss', 0)
                
                sss_value = 0
                if isinstance(cell.get('sss'), dict):
                    sss_value = cell['sss'].get('value', 0)
                else:
                    sss_value = cell.get('sss', 0)
                
                # Check PBCH decoded status
                pbch_decoded = False
                if cell.get('pbch') == 'Decoded':
                    pbch_decoded = True
                
                # Check SIB1 detected status
                sib1_detected = False
                if cell.get('sib1') == 'Detected':
                    sib1_detected = True
                
                try:
                    cell_pb = ue_service_pb2.CellInfo(
                        pci=int(cell.get('pci', 0)),
                        pss=int(pss_value),
                        sss=int(sss_value),
                        ss_rsrp=float(cell.get('ss_rsrp', -100.0)),
                        ss_rsrq=float(cell.get('ss_rsrq', -10.0)),
                        ss_sinr=float(cell.get('ss_sinr', 10.0)),
                        pbch_decoded=pbch_decoded,
                        mib=mib,
                        sib1_detected=sib1_detected,
                        sib1=sib1
                    )
                    cells.append(cell_pb)
                except Exception as e:
                    self.ue_process.log(f"Error creating CellInfo for cell {cell.get('id', 'unknown')}: {str(e)}", "error")
                    continue
            
            return ue_service_pb2.DetectedCellResponse(
                cells=cells,
                timestamp=int(time.time() * 1000)
            )
        except Exception as e:
            self.ue_process.log(f"Error in GetDetectedCells: {str(e)}", "error")
            return ue_service_pb2.DetectedCellResponse(
                cells=[],
                timestamp=int(time.time() * 1000)
            )
    
    

def signal_handler(signum, frame):
    """Handle shutdown signals"""
    logger.info("Received shutdown signal, stopping...")
    sys.exit(0)


def main():
    """Main function"""
    parser = argparse.ArgumentParser(description='Simulated UE Process')
    parser.add_argument('--port', type=int, default=50051, help='gRPC server port')
    parser.add_argument('--frequency', type=int, default=3425010000, help='Center frequency')
    
    # Authentication parameters
    parser.add_argument('--imsi', default='001010000000003', help='IMSI')
    parser.add_argument('--key', default='fec86ba6eb707ed08905757b1bb44b8f', help='Authentication key')
    parser.add_argument('--opc', default='c42449363bbad02b66d16bc975d77cc1', help='OPc value')
    
    # Network parameters
    parser.add_argument('--dnn', default='oai', help='Data Network Name')
    parser.add_argument('--nssai-sst', type=int, default=1, help='NSSAI SST value')
    parser.add_argument('--nssai-sd', type=int, default=1, help='NSSAI SD value')
    
    # Radio parameters
    parser.add_argument('--bandwidth', type=int, default=100, help='Bandwidth in MHz')
    parser.add_argument('--numerology', type=int, default=1, help='Numerology')
    
    args = parser.parse_args()
    
    # Set up signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Create and start UE process
    ue_process = SimulatedUEProcess(port=args.port)
    
    try:
        config = {
            'frequency': args.frequency,
            'imsi': args.imsi,
            'key': args.key,
            'opc': args.opc,
            'dnn': args.dnn,
            'nssai_sst': args.nssai_sst,
            'nssai_sd': args.nssai_sd,
            'bandwidth': args.bandwidth,
            'numerology': args.numerology
        }
        
        result = ue_process.start(config)
        logger.info(f"UE process started: {result}")
        
        # Keep the process running
        while ue_process.is_running:
            time.sleep(1)
            
    except KeyboardInterrupt:
        logger.info("Received keyboard interrupt")
    except Exception as e:
        logger.error(f"Error: {e}")
    finally:
        if ue_process.is_running:
            ue_process.stop()
        logger.info("Simulated UE process terminated")


if __name__ == '__main__':
    main()
