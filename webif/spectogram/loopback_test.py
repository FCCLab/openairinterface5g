#!/usr/bin/env python3
"""
loopback_test.py
UHD USRP RF loopback test (TX->RX), headless / no plots.

Features:
- Timed start for aligned TX/RX (disable with --no-timed-start)
- Lead-in trimming (drops startup zeros)
- Power (dBFS-ish), SNR estimate, tone offset, PPM error
- Optional RX/TX RF bandwidth setting (--set-bw)
- Optional IQ save to .npy (--save-iq path)
- CSV logging (--csv path)
- TX-gain sweep (--sweep "start:stop:step")
- Auto-level with real hardware gain limits:
    --autolevel -20 --autolevel-mode {tx,rx,both}  (default both)
- Digital amplitude control (--wave-amp 0.5)

Examples:
  python3 loopback_test.py --addr "addr=192.168.40.2" --rate 10e6 --freq 915e6 --tone-freq 100e3
  python3 loopback_test.py ... --autolevel -20 --autolevel-mode both
"""

import argparse, time, os, csv
import numpy as np
import uhd
import threading
from datetime import datetime

# ---------- Helpers ----------

def clamp(x, lo, hi):
    return max(lo, min(hi, x))

def make_tone(fs, f0, nsamps, amp=0.5):
    t = np.arange(nsamps, dtype=np.float64) / fs
    amp = float(np.clip(amp, 0.0, 1.0))
    return (amp * np.exp(2j * np.pi * f0 * t)).astype(np.complex64)

def estimate_snr_db(rx, fs, nfft=65536):
    n = min(len(rx), nfft)
    if n < 1024:
        return None, None
    win = np.hanning(n)
    spec = np.fft.fftshift(np.fft.fft(rx[:n] * win, n=nfft))
    freqs = np.fft.fftshift(np.fft.fftfreq(nfft, 1.0/fs))
    mag2 = np.abs(spec)**2
    pk = int(np.argmax(mag2))
    win_pow = np.sum(win**2) + 1e-20
    tone_pow = mag2[pk] / win_pow
    noise_idx = np.r_[0:max(pk-4,0), min(pk+5,nfft):nfft]
    noise_pow = np.mean(mag2[noise_idx] / win_pow + 1e-20)
    return 10*np.log10((tone_pow+1e-20)/(noise_pow+1e-20)), float(freqs[pk])

def parse_sweep(spec):
    try:
        start, stop, step = map(float, spec.split(":"))
        vals = []
        v = start
        if step == 0: raise ValueError("step must be non-zero")
        if step > 0:
            while v <= stop + 1e-9:
                vals.append(round(v,6)); v += step
        else:
            while v >= stop - 1e-9:
                vals.append(round(v,6)); v += step
        return vals
    except Exception as e:
        raise ValueError(f"Bad --sweep spec '{spec}': {e}")

# ---------- Threaded loopback test ----------

def tx_thread_function(usrp, tx_stream, tx_wave, args, stop_event):
    """
    TX thread function that continuously transmits the waveform.
    
    Args:
        usrp: USRP device object
        tx_stream: TX streamer object
        tx_wave: TX waveform to transmit
        args: Command line arguments
        stop_event: Threading event to signal stop
    """
    print("[TX] Starting continuous transmission...")
    
    try:
        # Start TX streaming
        tx_stream.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
        
        tx_md = uhd.types.TXMetadata()
        samps_sent = 0
        max_tx = tx_stream.get_max_num_samps()
        
        while not stop_event.is_set():
            # Send the waveform in chunks
            chunk = tx_wave[samps_sent % len(tx_wave):(samps_sent % len(tx_wave)) + max_tx]
            if len(chunk) < max_tx:
                # Wrap around to beginning of waveform
                remaining = max_tx - len(chunk)
                chunk = np.concatenate([chunk, tx_wave[:remaining]])
            
            sent = tx_stream.send(chunk, tx_md)
            samps_sent += sent
            
            # Small delay to prevent overwhelming the system
            time.sleep(0.001)
            
    except Exception as e:
        print(f"[TX] Error in TX thread: {e}")
    finally:
        print("[TX] Stopping transmission...")
        tx_stream.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))

def rx_thread_function(usrp, rx_stream, args, stop_event, results_queue):
    """
    RX thread function that continuously receives and processes samples.
    
    Args:
        usrp: USRP device object
        rx_stream: RX streamer object
        args: Command line arguments
        stop_event: Threading event to signal stop
        results_queue: Queue to store results
    """
    print("[RX] Starting continuous reception...")
    
    try:
        # Start RX streaming
        rx_stream.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
        
        rx_buf = np.empty((rx_stream.get_max_num_samps(),), np.complex64)
        rx_md = uhd.types.RXMetadata()
        frame_count = 0
        
        while not stop_event.is_set():
            try:
                # Receive samples
                n = rx_stream.recv(rx_buf, rx_md, timeout=0.1)
                if n > 0:
                    frame_count += 1
                    
                    # Process the received samples
                    samples = rx_buf[:n].copy()
                    
                    # Calculate metrics
                    pwr_dbfs = 10*np.log10(np.mean(np.abs(samples)**2)+1e-12)
                    snr_db, tone_est = estimate_snr_db(samples, args.rate)
                    
                    # Store results
                    result = {
                        "frame": frame_count,
                        "samples_received": n,
                        "pwr_dbfs": pwr_dbfs,
                        "snr_db": snr_db,
                        "tone_est_hz": tone_est,
                        "timestamp": time.time()
                    }
                    
                    results_queue.put(result)
                    
                    # Print status every 100 frames
                    if frame_count % 100 == 0:
                        snr_str = f"{snr_db:.1f} dB" if snr_db is not None else "n/a"
                        tone_str = f"{tone_est:.1f} Hz" if tone_est is not None else "n/a"
                        print(f"[RX] Frame {frame_count}: pwr={pwr_dbfs:.2f} dBFS, SNR={snr_str}, tone≈{tone_str}")
                        
            except Exception as e:
                print(f"[RX] Error receiving samples: {e}")
                time.sleep(0.1)
                
    except Exception as e:
        print(f"[RX] Error in RX thread: {e}")
    finally:
        print("[RX] Stopping reception...")
        rx_stream.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))

def run_threaded_loopback(usrp, rx_stream, tx_stream, tx_wave, args, duration=10):
    """
    Run threaded loopback test with continuous TX and RX.
    
    Args:
        usrp: USRP device object
        rx_stream: RX streamer object
        tx_stream: TX streamer object
        tx_wave: TX waveform
        args: Command line arguments
        duration: Test duration in seconds
    """
    import queue
    
    print(f"[INFO] Starting threaded loopback test for {duration} seconds...")
    
    # Create stop event and results queue
    stop_event = threading.Event()
    results_queue = queue.Queue()
    
    # Start TX and RX threads
    tx_thread = threading.Thread(target=tx_thread_function, 
                                args=(usrp, tx_stream, tx_wave, args, stop_event))
    rx_thread = threading.Thread(target=rx_thread_function, 
                                args=(usrp, rx_stream, args, stop_event, results_queue))
    
    tx_thread.daemon = True
    rx_thread.daemon = True
    
    tx_thread.start()
    rx_thread.start()
    
    # Let the test run for the specified duration
    start_time = time.time()
    results = []
    
    try:
        while time.time() - start_time < duration:
            try:
                # Get results from queue (non-blocking)
                result = results_queue.get_nowait()
                results.append(result)
            except queue.Empty:
                time.sleep(0.1)
                continue
    except KeyboardInterrupt:
        print("\n[INFO] Interrupted by user")
    
    # Stop the threads
    stop_event.set()
    tx_thread.join(timeout=2)
    rx_thread.join(timeout=2)
    
    # Print summary
    if results:
        avg_pwr = np.mean([r["pwr_dbfs"] for r in results])
        avg_snr = np.mean([r["snr_db"] for r in results if r["snr_db"] is not None])
        total_frames = len(results)
        
        print(f"\n[SUMMARY] Threaded loopback test completed:")
        print(f"  Total frames: {total_frames}")
        print(f"  Average power: {avg_pwr:.2f} dBFS")
        print(f"  Average SNR: {avg_snr:.1f} dB" if avg_snr is not None else "  Average SNR: n/a")
        print(f"  Duration: {time.time() - start_time:.1f} seconds")
    else:
        print("\n[SUMMARY] No results collected")

# ---------- USRP config ----------

def configure_usrp(usrp, args):
    usrp.set_clock_source("internal")
    usrp.set_time_now(uhd.types.TimeSpec(0.0))
    usrp.set_rx_rate(args.rate, args.chan)
    usrp.set_tx_rate(args.rate, args.chan)
    usrp.set_rx_freq(uhd.types.TuneRequest(args.freq), args.chan)
    usrp.set_tx_freq(uhd.types.TuneRequest(args.freq), args.chan)
    # Clamp gains to hardware range
    rx_gr = usrp.get_rx_gain_range(args.chan)
    tx_gr = usrp.get_tx_gain_range(args.chan)
    rx_gain_eff = clamp(args.rx_gain, rx_gr.start(), rx_gr.stop())
    tx_gain_eff = clamp(args.tx_gain, tx_gr.start(), tx_gr.stop())
    if rx_gain_eff != args.rx_gain:
        print(f"[info] RX gain {args.rx_gain} dB clamped to hardware range [{rx_gr.start():.1f},{rx_gr.stop():.1f}] -> {rx_gain_eff:.1f} dB")
    if tx_gain_eff != args.tx_gain:
        print(f"[info] TX gain {args.tx_gain} dB clamped to hardware range [{tx_gr.start():.1f},{tx_gr.stop():.1f}] -> {tx_gain_eff:.1f} dB")
    usrp.set_rx_gain(rx_gain_eff, args.chan)
    usrp.set_tx_gain(tx_gain_eff, args.chan)
    usrp.set_rx_antenna(args.rx_ant, args.chan)
    usrp.set_tx_antenna(args.tx_ant, args.chan)
    if args.set_bw:
        bw = min(args.rate, 20e6)
        usrp.set_rx_bandwidth(bw, args.chan)
        usrp.set_tx_bandwidth(bw, args.chan)
    return tx_gain_eff, rx_gain_eff

# ---------- One capture ----------

def run_once(usrp, rx_stream, tx_stream, tx_wave, args):
    rx_buf = np.empty((rx_stream.get_max_num_samps(),), np.complex64)
    rx_data = np.empty((args.nsamps,), np.complex64)
    rx_md = uhd.types.RXMetadata()

    # timed start
    tspec = None
    if not args.no_timed_start:
        start_time = usrp.get_time_now().get_real_secs() + 0.2
        tspec = uhd.types.TimeSpec(start_time)

    rx_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
    rx_cmd.num_samps = args.nsamps
    rx_cmd.stream_now = tspec is None
    if tspec: rx_cmd.time_spec = tspec
    rx_stream.issue_stream_cmd(rx_cmd)

    tx_md = uhd.types.TXMetadata()
    tx_md.has_time_spec = bool(tspec)
    if tspec: tx_md.time_spec = tspec

    if tspec:
        while usrp.get_time_now().get_real_secs() < tspec.get_real_secs() - 0.01:
            time.sleep(0.005)

    samps_sent = 0
    max_tx = tx_stream.get_max_num_samps()
    while samps_sent < len(tx_wave):
        chunk = tx_wave[samps_sent:samps_sent+max_tx]
        tx_md.end_of_burst = (samps_sent+len(chunk)) >= len(tx_wave)
        sent = tx_stream.send(chunk, tx_md)
        tx_md.has_time_spec = False
        samps_sent += sent

    total_rx = 0
    while total_rx < args.nsamps:
        n = rx_stream.recv(rx_buf, rx_md, timeout=2.0)
        if n > 0:
            rx_data[total_rx:total_rx+n] = rx_buf[:n]
            total_rx += n
        if rx_md.error_code != uhd.types.RXMetadataErrorCode.none:
            print(f"[RX] Error: {rx_md.strerror()}"); break

    lead_in = min(max(args.lead_in, 0), len(rx_data)//2)
    rx_use = rx_data[lead_in:]

    # metrics
    pwr_dbfs = 10*np.log10(np.mean(np.abs(rx_use)**2)+1e-12)
    snr_db, tone_est = estimate_snr_db(rx_use, args.rate)
    ppm = None
    if tone_est is not None and args.tone_freq != 0:
        ppm = (tone_est - args.tone_freq) / args.tone_freq * 1e6

    # clipping detector
    clip_frac = float(np.mean(np.abs(rx_use) >= 0.98))
    if clip_frac > 0:
        print(f"[warn] Clipping detected: {clip_frac*100:.3f}% of samples >= 0.98 FS")

    return {
        "tx_sent": samps_sent, "rx_got": total_rx,
        "lead_in": lead_in, "pwr_dbfs": pwr_dbfs,
        "snr_db": snr_db, "tone_est_hz": tone_est, "ppm": ppm
    }, rx_use

def append_csv(row, path):
    if not path: return
    exists = os.path.exists(path)
    with open(path,"a",newline="") as f:
        w = csv.DictWriter(f, fieldnames=row.keys())
        if not exists: w.writeheader()
        w.writerow(row)

# ---------- Auto-level ----------

def autolevel(usrp, rx_stream, tx_stream, tx_wave, args, target_dbfs, mode, max_iter=8):
    """
    Auto-level to target_dbfs by adjusting TX, RX, or both.
    Strategy:
      1) Adjust TX within hardware range toward target.
      2) If still hot and TX at minimum, reduce RX.
      3) If still hot and RX at minimum, suggest more attenuation or lower --wave-amp.
    Returns (tx_gain_final, rx_gain_final, result, rx_use)
    """
    tx_gr = usrp.get_tx_gain_range(args.chan)
    rx_gr = usrp.get_rx_gain_range(args.chan)

    def set_tx(g):
        g_eff = clamp(g, tx_gr.start(), tx_gr.stop())
        usrp.set_tx_gain(g_eff, args.chan)
        return g_eff

    def set_rx(g):
        g_eff = clamp(g, rx_gr.start(), rx_gr.stop())
        usrp.set_rx_gain(g_eff, args.chan)
        return g_eff

    # start from current
    tx_g = float(usrp.get_tx_gain(args.chan))
    rx_g = float(usrp.get_rx_gain(args.chan))

    for i in range(1, max_iter+1):
        result, rx_use = run_once(usrp, rx_stream, tx_stream, tx_wave, args)
        err = result['pwr_dbfs'] - target_dbfs
        print(f"[autolevel {i}/{max_iter}] tx={tx_g:.2f} dB, rx={rx_g:.2f} dB -> {result['pwr_dbfs']:.2f} dBFS (err {err:+.2f} dB)")
        if abs(err) <= 1.0:
            return tx_g, rx_g, result, rx_use

        step = 0.7 * err  # proportional step

        did_adjust = False
        if err > 0:  # too hot -> reduce gain
            if mode in ("tx", "both"):
                new_tx = set_tx(tx_g - step)
                did_adjust |= (abs(new_tx - tx_g) > 1e-6)
                tx_g = new_tx
            # If TX can't go lower or we still hot, try RX
            if (mode in ("rx", "both")) and (not did_adjust or tx_g <= tx_gr.start()+1e-3):
                new_rx = set_rx(rx_g - step)
                did_adjust |= (abs(new_rx - rx_g) > 1e-6)
                rx_g = new_rx
        else:  # too low -> increase gain
            if mode in ("tx", "both"):
                new_tx = set_tx(tx_g - step)  # err is negative, so -step increases
                did_adjust |= (abs(new_tx - tx_g) > 1e-6)
                tx_g = new_tx
            if (mode in ("rx", "both")) and (not did_adjust or tx_g >= tx_gr.stop()-1e-3):
                new_rx = set_rx(rx_g - step)
                did_adjust |= (abs(new_rx - rx_g) > 1e-6)
                rx_g = new_rx

        if not did_adjust:
            # Nowhere to go with RF gains
            print("[autolevel] Hit hardware gain limits. Consider lowering --wave-amp or adding more attenuation.")
            return tx_g, rx_g, result, rx_use

    return tx_g, rx_g, result, rx_use

# ---------- Main ----------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--addr", default="addr=192.168.40.2")
    ap.add_argument("--rate", type=float, default=10e6)
    ap.add_argument("--freq", type=float, default=915e6)
    ap.add_argument("--tone-freq", type=float, default=100e3)
    ap.add_argument("--wave-amp", type=float, default=0.5, help="Digital amplitude 0..1 for TX waveform")
    ap.add_argument("--tx-gain", type=float, default=0.0)
    ap.add_argument("--rx-gain", type=float, default=20.0)
    ap.add_argument("--tx-ant", default="TX/RX")
    ap.add_argument("--rx-ant", default="RX2")
    ap.add_argument("--nsamps", type=int, default=int(1e6))
    ap.add_argument("--chan", type=int, default=0)
    ap.add_argument("--lead-in", type=int, default=2000)
    ap.add_argument("--no-timed-start", action="store_true")
    ap.add_argument("--set-bw", action="store_true")
    ap.add_argument("--save-iq", default="")
    ap.add_argument("--csv", default="")
    ap.add_argument("--sweep", default="")
    ap.add_argument("--autolevel", type=float, default=None, help="Target dBFS (e.g., -20)")
    ap.add_argument("--autolevel-mode", choices=["tx","rx","both"], default="both",
                    help="Which gains to adjust during autolevel")
    args = ap.parse_args()

    usrp = uhd.usrp.MultiUSRP(args.addr)
    tx_gain_eff, rx_gain_eff = configure_usrp(usrp, args)
    print(f"[info] Effective gains -> TX: {tx_gain_eff:.1f} dB, RX: {rx_gain_eff:.1f} dB")

    tx_wave = make_tone(args.rate, args.tone_freq, args.nsamps, amp=args.wave_amp)

    rx_st_args = uhd.usrp.StreamArgs("fc32","sc16")
    tx_st_args = uhd.usrp.StreamArgs("fc32","sc16")
    rx_st_args.channels=[args.chan]; tx_st_args.channels=[args.chan]
    rx_stream = usrp.get_rx_stream(rx_st_args)
    tx_stream = usrp.get_tx_stream(tx_st_args)
    timestamp = datetime.now().isoformat(timespec="seconds")

    if args.sweep:
        gains = parse_sweep(args.sweep)
        for g in gains:
            gr = usrp.get_tx_gain_range(args.chan)
            g_eff = clamp(g, gr.start(), gr.stop())
            if g_eff != g:
                print(f"[info] tx_gain {g} dB clamped to [{gr.start():.1f},{gr.stop():.1f}] -> {g_eff:.1f} dB")
            usrp.set_tx_gain(g_eff, args.chan)
            result, rx_use = run_once(usrp, rx_stream, tx_stream, tx_wave, args)
            snr_str  = f"{result['snr_db']:.1f} dB" if result['snr_db'] is not None else "n/a"
            tone_str = f"{result['tone_est_hz']:.1f} Hz" if result['tone_est_hz'] is not None else "n/a"
            ppm_str  = f"{result['ppm']:+.2f} ppm" if result['ppm'] is not None else "n/a"
            print(f"[sweep] tx_gain={g_eff:>5.1f} dB -> pwr={result['pwr_dbfs']:.2f} dBFS, "
                  f"SNR={snr_str}, tone≈{tone_str}, {ppm_str}")
            if args.save_iq:
                np.save(args.save_iq.replace(".npy", f"_txg{g_eff:.1f}.npy"), rx_use)
            append_csv({
                "ts": timestamp, "mode": "sweep",
                "rate": args.rate, "freq": args.freq, "tone": args.tone_freq,
                "tx_gain": g_eff, "rx_gain": float(usrp.get_rx_gain(args.chan)),
                "pwr_dbfs": result["pwr_dbfs"], "snr_db": result["snr_db"],
                "tone_est_hz": result["tone_est_hz"], "ppm": result["ppm"],
                "nsamps": args.nsamps, "timed_start": int(not args.no_timed_start),
                "addr": args.addr, "tx_ant": args.tx_ant, "rx_ant": args.rx_ant,
                "wave_amp": args.wave_amp
            }, args.csv)

    elif args.autolevel is not None:
        tx_f, rx_f, result, rx_use = autolevel(
            usrp, rx_stream, tx_stream, tx_wave, args,
            target_dbfs=args.autolevel, mode=args.autolevel_mode
        )
        snr_str  = f"{result['snr_db']:.1f} dB" if result['snr_db'] is not None else "n/a"
        tone_str = f"{result['tone_est_hz']:.1f} Hz" if result['tone_est_hz'] is not None else "n/a"
        ppm_str  = f"{result['ppm']:+.2f} ppm" if result['ppm'] is not None else "n/a"
        print(f"[autolevel] final TX={tx_f:.2f} dB, RX={rx_f:.2f} dB -> pwr={result['pwr_dbfs']:.2f} dBFS, "
              f"SNR={snr_str}, tone≈{tone_str}, {ppm_str}")

        if args.save_iq:
            np.save(args.save_iq, rx_use)
            print(f"[file] Saved IQ to {args.save_iq}")

        append_csv({
            "ts": timestamp, "mode": "autolevel",
            "rate": args.rate, "freq": args.freq, "tone": args.tone_freq,
            "tx_gain": tx_f, "rx_gain": rx_f,
            "pwr_dbfs": result["pwr_dbfs"], "snr_db": result["snr_db"],
            "tone_est_hz": result["tone_est_hz"], "ppm": result["ppm"],
            "nsamps": args.nsamps, "timed_start": int(not args.no_timed_start),
            "addr": args.addr, "tx_ant": args.tx_ant, "rx_ant": args.rx_ant,
            "wave_amp": args.wave_amp
        }, args.csv)

    else:
        result, rx_use = run_once(usrp, rx_stream, tx_stream, tx_wave, args)
        snr_str  = f"{result['snr_db']:.1f} dB" if result['snr_db'] is not None else "n/a"
        tone_str = f"{result['tone_est_hz']:.1f} Hz" if result['tone_est_hz'] is not None else "n/a"
        ppm_str  = f"{result['ppm']:+.2f} ppm" if result['ppm'] is not None else "n/a"
        print(f"TX sent {result['tx_sent']}, RX got {result['rx_got']}")
        print("First 8 RX samples:", rx_use[:8])
        print(f"Trimmed RX power: {result['pwr_dbfs']:.2f} dBFS")
        print(f"SNR ≈ {snr_str}, tone offset ≈ {tone_str}, {ppm_str}")

        if args.save_iq:
            np.save(args.save_iq, rx_use)
            print(f"[file] Saved IQ to {args.save_iq}")

        append_csv({
            "ts": timestamp, "mode": "single",
            "rate": args.rate, "freq": args.freq, "tone": args.tone_freq,
            "tx_gain": float(usrp.get_tx_gain(args.chan)),
            "rx_gain": float(usrp.get_rx_gain(args.chan)),
            "pwr_dbfs": result["pwr_dbfs"], "snr_db": result["snr_db"],
            "tone_est_hz": result["tone_est_hz"], "ppm": result["ppm"],
            "nsamps": args.nsamps, "timed_start": int(not args.no_timed_start),
            "addr": args.addr, "tx_ant": args.tx_ant, "rx_ant": args.rx_ant,
            "wave_amp": args.wave_amp
        }, args.csv)

if __name__ == "__main__":
    main()