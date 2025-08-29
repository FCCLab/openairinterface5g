#!/usr/bin/env python3
"""
Generate window function plots for STFT guide
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy import signal

def plot_window_functions():
    """Generate and save window function plots"""
    
    # Window length
    N = 256
    
    # Create sample points
    n = np.arange(N)
    
    # Generate different window functions
    hann = signal.windows.hann(N)
    hamming = signal.windows.hamming(N)
    blackman = signal.windows.blackman(N)
    
    # Create the plot
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6))
    
    # Individual windows
    ax1.plot(n, hann, 'b-', linewidth=2, label='Hann Window')
    ax1.plot(n, hamming, 'r-', linewidth=2, label='Hamming Window')
    ax1.plot(n, blackman, 'g-', linewidth=2, label='Blackman Window')
    ax1.set_xlabel('Sample Index (n)')
    ax1.set_ylabel('Amplitude')
    ax1.set_title('Window Functions Comparison')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.set_xlim(0, N-1)
    ax1.set_ylim(0, 1.1)
    
    # Frequency domain (magnitude response)
    # Calculate frequency response
    freqs = np.linspace(0, np.pi, 1024)
    
    # Get frequency responses
    _, hann_response = signal.freqz(hann, 1, worN=freqs)
    _, hamming_response = signal.freqz(hamming, 1, worN=freqs)
    _, blackman_response = signal.freqz(blackman, 1, worN=freqs)
    
    # Convert to dB
    hann_db = 20 * np.log10(np.abs(hann_response))
    hamming_db = 20 * np.log10(np.abs(hamming_response))
    blackman_db = 20 * np.log10(np.abs(blackman_response))
    
    # Plot frequency responses
    ax2.plot(freqs, hann_db, 'b-', linewidth=2, label='Hann Window')
    ax2.plot(freqs, hamming_db, 'r-', linewidth=2, label='Hamming Window')
    ax2.plot(freqs, blackman_db, 'g-', linewidth=2, label='Blackman Window')
    ax2.set_xlabel('Normalized Frequency (π rad/sample)')
    ax2.set_ylabel('Magnitude (dB)')
    ax2.set_title('Frequency Response of Window Functions')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.set_xlim(0, np.pi)
    ax2.set_ylim(-100, 10)
    
    # Add frequency labels
    ax2.set_xticks([0, np.pi/4, np.pi/2, 3*np.pi/4, np.pi])
    ax2.set_xticklabels(['0', 'π/4', 'π/2', '3π/4', 'π'])
    
    plt.tight_layout()
    
    # Save the plot
    plt.savefig('window_functions.png', dpi=150, bbox_inches='tight', 
                facecolor='white', edgecolor='none')
    plt.close()
    
    print("Window functions plot saved as 'window_functions.png'")

if __name__ == "__main__":
    plot_window_functions()
