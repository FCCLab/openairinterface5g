#!/usr/bin/env python3
"""
Test gRPC client for Simulated UE Process
"""

import grpc
import time
import json
import sys

# Import generated protobuf classes
try:
    import ue_service_pb2
    import ue_service_pb2_grpc
except ImportError as e:
    print(f"Error importing protobuf files: {e}")
    print("Please make sure ue_service_pb2.py and ue_service_pb2_grpc.py are generated")
    sys.exit(1)

def test_grpc_client():
    """Test the gRPC client functionality"""
    print("Testing gRPC client for Simulated UE Process...\n")
    
    # Create gRPC channel
    channel = grpc.insecure_channel('localhost:50051')
    
    try:
        # Test 1: Get Status
        print("1. Testing GetStatus...")
        control_stub = ue_service_pb2_grpc.UEControlServiceStub(channel)
        
        status_request = ue_service_pb2.StatusRequest(process_id="test")
        status_response = control_stub.GetStatus(status_request)
        
        print(f"Status Response:")
        print(f"  Running: {status_response.is_running}")
        print(f"  Status: {status_response.status}")
        print(f"  Connection State: {status_response.connection_state}")
        print(f"  Frequency: {status_response.frequency}")
        print(f"  Uptime: {status_response.uptime}")
        print()
        
        # Test 2: Start UE
        print("2. Testing StartUE...")
        start_request = ue_service_pb2.StartUERequest(
            frequency="3425010000",
            config=ue_service_pb2.UEConfig(
                frequency="3425010000",
                auth=ue_service_pb2.AuthenticationConfig(
                    imsi="001010000000003",
                    key="fec86ba6eb707ed08905757b1bb44b8f",
                    opc="c42449363bbad02b66d16bc975d77cc1"
                ),
                network=ue_service_pb2.NetworkConfig(
                    mcc="001",
                    mnc="01",
                    tac="1"
                ),
                radio=ue_service_pb2.RadioConfig(
                    frequency="3425010000",
                    bandwidth="100",
                    power="23"
                )
            )
        )
        
        start_response = control_stub.StartUE(start_request)
        print(f"Start Response:")
        print(f"  Success: {start_response.success}")
        print(f"  Message: {start_response.message}")
        print(f"  Process ID: {start_response.process_id}")
        print()
        
        # Test 3: Get Current Cells
        print("3. Testing GetCurrentCells...")
        data_stub = ue_service_pb2_grpc.UEDataServiceStub(channel)
        
        cells_request = ue_service_pb2.CurrentCellsRequest(process_id="test")
        cells_response = data_stub.GetCurrentCells(cells_request)
        
        print(f"Cells Response:")
        print(f"  Timestamp: {cells_response.timestamp}")
        print(f"  Number of cells: {len(cells_response.cells)}")
        
        for i, cell in enumerate(cells_response.cells[:3]):  # Show first 3 cells
            print(f"  Cell {i+1}:")
            print(f"    PCI: {cell.pci}")
            print(f"    PSS: {cell.pss}")
            print(f"    SSS: {cell.sss}")
            print(f"    SS-RSRP: {cell.ss_rsrp:.2f} dBm")
            print(f"    SS-RSRQ: {cell.ss_rsrq:.2f} dB")
            print(f"    SS-SINR: {cell.ss_sinr:.2f} dB")
            print(f"    PBCH Decoded: {cell.pbch_decoded}")
            print(f"    SIB1 Detected: {cell.sib1_detected}")
        print()
        
        # Test 4: Stream Cell Data (for 10 seconds)
        print("4. Testing StreamCellData (10 seconds)...")
        stream_request = ue_service_pb2.CellDataRequest(
            process_id="test",
            continuous=True
        )
        
        cell_data_count = 0
        start_time = time.time()
        
        try:
            for response in data_stub.StreamCellData(stream_request):
                cell_data_count += 1
                elapsed = time.time() - start_time
                
                print(f"Cell Data Update #{cell_data_count} (t={elapsed:.1f}s):")
                print(f"  Timestamp: {response.timestamp}")
                print(f"  Cell count: {len(response.cells)}")
                if response.cells:
                    print(f"  Strongest PCI: {response.cells[0].pci}")
                    print(f"  Strongest RSRP: {response.cells[0].ss_rsrp:.2f} dBm")
                print()
                
                if elapsed >= 10:
                    break
                    
        except grpc.RpcError as e:
            print(f"Stream error: {e}")
        
        print(f"Received {cell_data_count} cell data updates in 10 seconds\n")
        
        # Test 5: Stream Logs (for 5 seconds)
        print("5. Testing StreamLogs (5 seconds)...")
        log_request = ue_service_pb2.LogRequest(
            process_id="test",
            level="INFO"
        )
        
        log_count = 0
        start_time = time.time()
        
        try:
            for log_response in data_stub.StreamLogs(log_request):
                log_count += 1
                elapsed = time.time() - start_time
                
                print(f"Log #{log_count} (t={elapsed:.1f}s): [{log_response.level}] {log_response.message}")
                
                if elapsed >= 5:
                    break
                    
        except grpc.RpcError as e:
            print(f"Log stream error: {e}")
        
        print(f"Received {log_count} log entries in 5 seconds\n")
        
        # Test 6: Stop UE
        print("6. Testing StopUE...")
        stop_request = ue_service_pb2.StopUERequest(process_id="test")
        stop_response = control_stub.StopUE(stop_request)
        
        print(f"Stop Response:")
        print(f"  Success: {stop_response.success}")
        print(f"  Message: {stop_response.message}")
        print()
        
        print("All tests completed successfully!")
        
    except grpc.RpcError as e:
        print(f"gRPC error: {e.code()} - {e.details()}")
    except Exception as e:
        print(f"Test error: {e}")
    finally:
        channel.close()

if __name__ == '__main__':
    test_grpc_client()
