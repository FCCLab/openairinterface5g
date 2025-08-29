import numpy as np
import uhd
import time
from scipy import signal

def read_iq_and_stft(
    center_freq=2.4e9,
    sample_rate=1e6,
    fft_size=1024,
    resolution=None,
    window_size=None,
    hop_size=None,
    window_type='hann',
    gain=20,
    device_addr=""
):
    """
    Use USRP UHD to read IQ samples and perform STFT.

    Args:
        center_freq (float): Center frequency in Hz.
        sample_rate (float): Sample rate (bandwidth) in Hz.
        fft_size (int): FFT size (number of frequency bins).
        resolution (float): Desired frequency resolution in Hz (optional, overrides fft_size if provided).
        window_size (int): Window size in samples (optional, defaults to fft_size).
        hop_size (int): Hop size in samples (optional, defaults to fft_size/2).
        window_type (str): Window type ('hann', 'hamming', 'blackman', 'rectangular').
        gain (float): Gain in dB.
        device_addr (str): UHD device address string (e.g., "addr=192.168.10.2").

    Returns:
        dict: {
            'success': bool,
            'stft': np.ndarray,
            'iq': np.ndarray,
            'frequencies': np.ndarray,
            'times': np.ndarray,
            'error': str (if any)
        }
    """
    try:
        # Calculate FFT size from resolution if provided, otherwise use provided fft_size
        if resolution is not None:
            # Calculate required FFT size from desired resolution
            # resolution = sample_rate / fft_size
            # fft_size = sample_rate / resolution
            calculated_fft_size = int(sample_rate / resolution)
            print(f"00 spectrogram.py: Calculated FFT size from resolution: {calculated_fft_size}")
        else:
            calculated_fft_size = fft_size
            print(f"00 spectrogram.py: Using provided FFT size: {fft_size}")
        
        # Ensure FFT size is a power of 2 for efficient FFT computation
        if not (calculated_fft_size & (calculated_fft_size - 1) == 0):
            # Find nearest power of 2
            original_fft_size = calculated_fft_size
            calculated_fft_size = 2 ** int(np.log2(calculated_fft_size))
            print(f"00 spectrogram.py: FFT size adjusted to power of 2: {original_fft_size} -> {calculated_fft_size}")
        
        if calculated_fft_size < 64:
            calculated_fft_size = 64
            print(f"00 spectrogram.py: FFT size too small, set to minimum: {calculated_fft_size}")
        elif calculated_fft_size > 8192:
            calculated_fft_size = 8192
            print(f"00 spectrogram.py: FFT size too large, set to maximum: {calculated_fft_size}")
        
        # Set window size (default to FFT size if not specified)
        if window_size is None:
            window_size = calculated_fft_size
        print(f"00 spectrogram.py: Window size: {window_size} samples")
        
        # Set hop size (default to window_size/2 if not specified)
        if hop_size is None:
            hop_size = window_size // 2
        print(f"00 spectrogram.py: Hop size: {hop_size} samples")
        
        # Calculate overlap samples for STFT
        overlap_samples = window_size - hop_size
        # Ensure overlap is less than window size
        if overlap_samples >= window_size:
            overlap_samples = window_size - 1
            hop_size = 1
            print(f"00 spectrogram.py: Overlap adjusted to {overlap_samples} samples (max allowed)")
        
        # Calculate total number of samples (enough for multiple STFT frames)
        num_samples = window_size * 4  # At least 4 window lengths for good STFT
        
        # Calculate actual frequency resolution
        actual_resolution = sample_rate / calculated_fft_size
        
        print(f"00 spectrogram.py: FFT size: {calculated_fft_size} samples, Resolution: {actual_resolution:.0f} Hz")
        print(f"00 spectrogram.py: Window size: {window_size} samples, Hop size: {hop_size} samples")
        print(f"00 spectrogram.py: Window type: {window_type}")
        
        # Create USRP device
        usrp = uhd.usrp.MultiUSRP(device_addr) if device_addr else uhd.usrp.MultiUSRP()
        
        # Print device information
        print(f"00 spectrogram.py: USRP device: {usrp.get_pp_string()}")
        print(f"00 spectrogram.py: Device time: {usrp.get_time_now()}")
        
        # Set parameters
        usrp.set_rx_rate(sample_rate)  # sample rate
        usrp.set_rx_freq(center_freq)
        usrp.set_rx_gain(gain)
        
        # Verify settings
        print(f"00 spectrogram.py: Sample rate: {usrp.get_rx_rate()}")
        print(f"00 spectrogram.py: Center frequency: {usrp.get_rx_freq()}")
        print(f"00 spectrogram.py: Gain: {usrp.get_rx_gain()}")
        # Allow for tuning/settling
        time.sleep(0.5)
        
        # Setup RX streamer
        st_args = uhd.usrp.StreamArgs("fc32", "fc32")
        rx_streamer = usrp.get_rx_stream(st_args)
        
        # Start streaming
        rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
        time.sleep(0.1)  # Allow streaming to start
        buffer_samps = min(num_samples, rx_streamer.get_max_num_samps())
        rx_buffer = np.zeros((buffer_samps,), dtype=np.complex64)

        md = uhd.types.RXMetadata()
        samps_recvd = 0
        iq_samples = []

        while samps_recvd < num_samples:
            num_to_recv = min(buffer_samps, num_samples - samps_recvd)
            # Create a buffer for this specific receive operation
            recv_buffer = np.zeros((num_to_recv,), dtype=np.complex64)
            n = rx_streamer.recv(recv_buffer, md, timeout=5.0)  # Increased timeout to 5 seconds
            if md.error_code != uhd.types.RXMetadataErrorCode.none:
                if md.error_code == uhd.types.RXMetadataErrorCode.timeout:
                    print(f"00 spectrogram.py: Timeout after {samps_recvd} samples received")
                    # If we have some data, continue with what we have
                    if samps_recvd > 0:
                        break
                    else:
                        return {
                            'success': False,
                            'error': f"RX timeout: No data received from device",
                            'iq': None,
                            'stft': None,
                            'frequencies': None,
                            'times': None
                        }
                else:
                    return {
                        'success': False,
                        'error': f"RX error: {md.strerror()}",
                        'iq': None,
                        'stft': None,
                        'frequencies': None,
                        'times': None
                    }
            iq_samples.append(recv_buffer[:n].copy())
            samps_recvd += n
            print(f"00 spectrogram.py: Received {n} samples, total: {samps_recvd}/{num_samples}")

        iq_data = np.concatenate(iq_samples)[:num_samples]

        # Perform STFT
        if len(iq_data) < window_size:
            # Zero pad if not enough samples
            iq_data = np.pad(iq_data, (0, window_size - len(iq_data)), 'constant')
        
        # Use scipy.signal.stft for proper STFT computation
        frequencies, times, stft_result = signal.stft(
            iq_data, 
            fs=sample_rate,  # Sample rate
            window=window_type,  # Window type
            nperseg=window_size,  # Window size
            noverlap=overlap_samples,  # Overlap samples
            nfft=calculated_fft_size,  # FFT size
            return_onesided=False  # Return full spectrum (negative and positive frequencies)
        )
        
        # Convert to magnitude spectrum in dB
        stft_magnitude = 20 * np.log10(np.abs(stft_result) + 1e-12)
        
        # Shift frequencies to center around 0 Hz
        frequencies = np.fft.fftshift(frequencies)
        stft_magnitude = np.fft.fftshift(stft_magnitude, axes=0)

        return {
            'success': True,
            'iq': iq_data,
            'stft': stft_magnitude,
            'frequencies': frequencies,
            'times': times,
            'error': None
        }
    except Exception as e:
        print(f"00 spectrogram.py: read_iq_and_fft() failed: {str(e)}")
        return {
            'success': False,
            'error': str(e),
            'iq': None,
            'fft': None
        }

# Example usage of read_iq_and_fft for quick test/demo
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Run IQ and STFT capture example")
    parser.add_argument("--device", type=str, default="", help="USRP device address (e.g., serial=xxxx)")
    parser.add_argument("--freq", type=float, default=2.4e9, help="Center frequency in Hz")
    parser.add_argument("--sample_rate", type=float, default=1e6, help="Sample rate (bandwidth) in Hz")
    parser.add_argument("--fft_size", type=int, default=1024, help="FFT size (number of frequency bins)")
    parser.add_argument("--resolution", type=float, default=None, help="Desired frequency resolution in Hz (overrides fft_size if provided)")
    parser.add_argument("--window_size", type=int, default=None, help="Window size in samples (defaults to fft_size)")
    parser.add_argument("--hop_size", type=int, default=None, help="Hop size in samples (defaults to window_size/2)")
    parser.add_argument("--window_type", type=str, default="hann", choices=["hann", "hamming", "blackman", "rectangular"], help="Window type")
    parser.add_argument("--gain", type=float, default=20, help="Gain in dB")
    parser.add_argument("--save_plot", type=str, default="", help="Save plot to file (e.g., 'spectrogram.png')")
    parser.add_argument("--test", action="store_true", help="Run with simulated data (no USRP required)")
    args = parser.parse_args()

    if args.test:
        # Generate simulated data for testing
        print("00 spectrogram.py: Running in test mode with simulated data")
        import numpy as np
        
        # Generate simulated IQ data
        num_samples = args.fft_size * 4
        t = np.linspace(0, num_samples/args.sample_rate, num_samples)
        
        # Create some test signals
        signal1 = np.exp(1j * 2 * np.pi * 0.1e6 * t)  # 100 kHz signal
        signal2 = np.exp(1j * 2 * np.pi * 0.3e6 * t)  # 300 kHz signal
        noise = (np.random.randn(num_samples) + 1j * np.random.randn(num_samples)) * 0.1
        
        iq_data = signal1 + signal2 + noise
        
        # Perform STFT on simulated data
        window_size = args.window_size if args.window_size else args.fft_size
        hop_size = args.hop_size if args.hop_size else window_size // 2
        overlap_samples = window_size - hop_size
        
        frequencies, times, stft_result = signal.stft(
            iq_data, 
            fs=args.sample_rate,
            window=args.window_type,
            nperseg=window_size,
            noverlap=overlap_samples,
            nfft=args.fft_size,
            return_onesided=False
        )
        
        stft_magnitude = 20 * np.log10(np.abs(stft_result) + 1e-12)
        frequencies = np.fft.fftshift(frequencies)
        stft_magnitude = np.fft.fftshift(stft_magnitude, axes=0)
        
        result = {
            'success': True,
            'iq': iq_data,
            'stft': stft_magnitude,
            'frequencies': frequencies,
            'times': times,
            'error': None
        }
    else:
        result = read_iq_and_stft(
            device_addr=args.device,
            center_freq=args.freq,
            sample_rate=args.sample_rate,
            fft_size=args.fft_size,
            resolution=args.resolution,
            window_size=args.window_size,
            hop_size=args.hop_size,
            window_type=args.window_type,
            gain=args.gain
        )

    if not result['success']:
        print(f"00 spectrogram.py: Example failed: {result['error']}")
    else:
        print("00 spectrogram.py: Example succeeded.")
        print(f"  Center Frequency: {args.freq/1e9:.3f} GHz")
        print(f"  Sample Rate: {args.sample_rate/1e6:.1f} MHz")
        print(f"  FFT Size: {args.fft_size} bins")
        if args.resolution is not None:
            print(f"  Resolution: {args.resolution/1e3:.1f} kHz (calculated)")
        else:
            print(f"  Resolution: {args.sample_rate/args.fft_size/1e3:.1f} kHz")
        print(f"  Window Size: {args.window_size if args.window_size else args.fft_size} samples")
        print(f"  Hop Size: {args.hop_size if args.hop_size else (args.window_size if args.window_size else args.fft_size)//2} samples")
        print(f"  Window Type: {args.window_type}")
        print(f"  IQ shape: {result['iq'].shape}")
        print(f"  STFT shape: {result['stft'].shape}")
        print(f"  Frequencies: {len(result['frequencies'])} bins")
        print(f"  Time frames: {len(result['times'])} frames")
        
        # Optionally plot STFT if matplotlib is available
        try:
            import matplotlib.pyplot as plt
            import matplotlib.colors as colors
            
            # Create figure with subplots
            fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
            # Adjust subplot heights manually
            ax1.set_position([0.1, 0.35, 0.8, 0.55])  # [left, bottom, width, height]
            ax2.set_position([0.1, 0.1, 0.8, 0.2])
            
            # Calculate frequency range for better display
            freq_range = result['frequencies'][-1] - result['frequencies'][0]
            freq_center = (result['frequencies'][-1] + result['frequencies'][0]) / 2
            
            # Create custom colormap for better visualization
            colors_list = ['black', 'darkblue', 'blue', 'cyan', 'yellow', 'orange', 'red', 'white']
            n_bins = 256
            cmap = colors.LinearSegmentedColormap.from_list('spectrum', colors_list, N=n_bins)
            
            # Plot STFT as spectrogram
            im = ax1.imshow(result['stft'], 
                           aspect='auto', 
                           origin='lower',
                           cmap=cmap,
                           vmin=-80, vmax=0,  # dB range
                           extent=[result['times'][0], result['times'][-1], 
                                   result['frequencies'][0]/1e6, result['frequencies'][-1]/1e6])
            
            # Add colorbar
            cbar = plt.colorbar(im, ax=ax1, label='Magnitude (dB)')
            cbar.set_label('Magnitude (dB)', rotation=270, labelpad=20)
            
            # Set title and labels
            ax1.set_title(f"STFT Spectrogram - {args.freq/1e9:.3f} GHz ± {args.bandwidth/2e6:.1f} MHz\n"
                         f"Window: {args.window_len} samples, Overlap: {args.overlap:.1%}, "
                         f"Resolution: {args.bandwidth/args.window_len/1e3:.1f} kHz", 
                         fontsize=12, fontweight='bold')
            ax1.set_xlabel("Time (s)")
            ax1.set_ylabel("Frequency Offset (MHz)")
            
            # Add grid
            ax1.grid(True, alpha=0.3)
            
            # Add frequency markers
            freq_step = max(1, len(result['frequencies']) // 10)  # Show ~10 frequency markers
            for i in range(0, len(result['frequencies']), freq_step):
                freq_mhz = result['frequencies'][i] / 1e6
                ax1.axhline(y=freq_mhz, color='white', alpha=0.3, linewidth=0.5)
                ax1.text(result['times'][-1] + 0.01, freq_mhz, f'{freq_mhz:.1f}', 
                        fontsize=8, verticalalignment='center', color='white')
            
            # Plot time-averaged spectrum in the second subplot
            avg_spectrum = np.mean(result['stft'], axis=1)
            ax2.plot(result['frequencies']/1e6, avg_spectrum, 'b-', linewidth=2, label='Average Spectrum')
            ax2.set_xlabel("Frequency Offset (MHz)")
            ax2.set_ylabel("Magnitude (dB)")
            ax2.set_title("Time-Averaged Frequency Spectrum")
            ax2.grid(True, alpha=0.3)
            ax2.legend()
            
            # Find and mark peak frequency
            peak_idx = np.argmax(avg_spectrum)
            peak_freq = result['frequencies'][peak_idx] / 1e6
            peak_mag = avg_spectrum[peak_idx]
            ax2.plot(peak_freq, peak_mag, 'ro', markersize=8, label=f'Peak: {peak_freq:.2f} MHz')
            ax2.annotate(f'{peak_mag:.1f} dB', (peak_freq, peak_mag), 
                        xytext=(10, 10), textcoords='offset points', 
                        bbox=dict(boxstyle='round,pad=0.3', facecolor='yellow', alpha=0.7))
            
            # Add statistics text
            stats_text = (f"Max: {np.max(avg_spectrum):.1f} dB\n"
                         f"Min: {np.min(avg_spectrum):.1f} dB\n"
                         f"Mean: {np.mean(avg_spectrum):.1f} dB\n"
                         f"Std: {np.std(avg_spectrum):.1f} dB")
            ax2.text(0.02, 0.98, stats_text, transform=ax2.transAxes, 
                    verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
            
            # Save plot if requested
            if args.save_plot:
                try:
                    # Use lower DPI to avoid large image size issues
                    plt.savefig(args.save_plot, dpi=150, bbox_inches='tight')
                    print(f"📁 Plot saved to: {args.save_plot}")
                except Exception as e:
                    print(f"❌ Failed to save plot: {str(e)}")
                    # Try with even lower DPI
                    try:
                        plt.savefig(args.save_plot, dpi=72, bbox_inches='tight')
                        print(f"📁 Plot saved to: {args.save_plot} (low resolution)")
                    except Exception as e2:
                        print(f"❌ Failed to save plot even with low resolution: {str(e2)}")
            
            plt.show()
            
            # Print additional statistics
            print(f"\n📊 Spectrogram Statistics:")
            print(f"  Peak Frequency: {peak_freq:.2f} MHz ({peak_mag:.1f} dB)")
            print(f"  Frequency Range: {result['frequencies'][0]/1e6:.1f} to {result['frequencies'][-1]/1e6:.1f} MHz")
            print(f"  Time Duration: {result['times'][-1] - result['times'][0]:.3f} seconds")
            print(f"  Dynamic Range: {np.max(avg_spectrum) - np.min(avg_spectrum):.1f} dB")
            
        except ImportError:
            print("00 spectrogram.py: matplotlib not installed, skipping plot.")
        except Exception as e:
            print(f"00 spectrogram.py: Plotting error: {str(e)}")
            print("00 spectrogram.py: Skipping plot due to error.")
