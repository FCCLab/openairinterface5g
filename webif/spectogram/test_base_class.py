#!/usr/bin/env python3
"""
Test script for the ProcessBase class.
This script tests the basic functionality of the base class.
"""

import multiprocessing
import time
from process_base import ProcessBase


class TestProcess(ProcessBase):
    """Test process that inherits from ProcessBase"""
    
    def __init__(self, stop_event, timestamp, cpu_core=0):
        super().__init__("Test", stop_event, timestamp, cpu_core)
        
        # Set custom memory threshold for testing
        self.set_memory_threshold(100)  # 100MB for testing
        self.set_memory_check_interval(5.0)  # Check every 5 seconds
        
        self.logger.info("Test process initialized with custom settings")
    
    def run(self):
        """Main test loop"""
        self.logger.info("Test process started")
        
        frame_count = 0
        start_time = time.time()
        
        while not self.stop_event.is_set():
            try:
                current_time = time.time()
                
                # Simulate some work
                time.sleep(0.1)
                frame_count += 1
                self.frame_count += 1
                
                # Use base class methods
                self._check_memory_periodic(current_time)
                self._log_fps(current_time)
                
                # Simulate memory usage growth (for testing)
                if frame_count % 50 == 0:
                    # Create some temporary objects to test memory monitoring
                    temp_data = [i for i in range(10000)]
                    self.logger.debug(f"Created temporary data, frame {frame_count}")
                    del temp_data  # Clean up immediately
                
                # Check parent process health
                if not self._check_parent_alive():
                    self.logger.warning("Parent process check failed")
                    break
                
                # Test for 10 seconds
                if current_time - start_time > 10:
                    self.logger.info("Test completed successfully")
                    break
                    
            except Exception as e:
                self.logger.error(f"Error in test loop: {e}")
                break
        
        return True


def main():
    """Main test function"""
    print("Testing ProcessBase class...")
    
    # Create stop event
    stop_event = multiprocessing.Event()
    
    # Create test process
    process = TestProcess(stop_event, "test_base", cpu_core=0)
    
    try:
        # Start the process
        print("Starting test process...")
        success = process.start()
        
        if success:
            print("✅ Test process completed successfully")
        else:
            print("❌ Test process failed")
            
    except KeyboardInterrupt:
        print("\nReceived keyboard interrupt, stopping...")
        stop_event.set()
    except Exception as e:
        print(f"❌ Error during test: {e}")
    finally:
        # Clean up
        if not stop_event.is_set():
            stop_event.set()
        print("Test completed")


if __name__ == "__main__":
    main()
