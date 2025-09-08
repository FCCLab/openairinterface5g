# UE Backend API Documentation

This document describes the UE (User Equipment) backend API endpoints for the OpenAirInterface5G web interface.

## Base URL
```
http://localhost:40000/api/ue
```

## Authentication
Currently, no authentication is required. In production, consider implementing proper authentication mechanisms.

## Endpoints

### 1. UE Status
**GET** `/status`

Get the current status of the UE process.

**Response:**
```json
{
  "connected": false,
  "state": "disconnected",
  "processId": null,
  "startTime": null,
  "lastActivity": "2024-01-15T10:30:00.000Z",
  "error": null,
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

**States:**
- `disconnected`: UE is not running
- `starting`: UE is initializing
- `scanning`: UE is performing cell search
- `connected`: UE is connected to network
- `error`: UE encountered an error

### 2. UE Configuration
**GET** `/config`

Get the current UE configuration.

**Response:**
```json
{
  "imsi": "001010000000003",
  "key": "fec86ba6eb707ed08905757b1bb44b8f",
  "opc": "c42449363bbad02b66d16bc975d77cc1",
  "dnn": "oai",
  "nssai_sst": 1,
  "usrpArgs": "type=x300,addr=192.168.40.2,clock=internal,time=internal",
  "frequency": 3425010000,
  "numerology": 1,
  "band": 78
}
```

**PUT** `/config`

Update UE configuration.

**Request Body:**
```json
{
  "frequency": 3500000000,
  "imsi": "001010000000004",
  "band": 78
}
```

**Response:**
```json
{
  "success": true,
  "config": {
    // Updated configuration object
  }
}
```

### 3. UE Process Control

**POST** `/start`

Start the UE process.

**Request Body (optional):**
```json
{
  "frequency": 3500000000,
  "band": 78
}
```

**Response:**
```json
{
  "success": true,
  "message": "UE process started",
  "pid": 12345,
  "config": {
    // Current configuration
  }
}
```

**POST** `/stop`

Stop the UE process.

**Response:**
```json
{
  "success": true,
  "message": "UE process stopped"
}
```

**POST** `/restart`

Restart the UE process.

**Request Body (optional):**
```json
{
  "frequency": 3500000000
}
```

**Response:**
```json
{
  "success": true,
  "message": "UE process started",
  "pid": 12346,
  "config": {
    // Current configuration
  }
}
```

### 4. Cell Scanning

**POST** `/scan/start`

Start a cell scan with the specified center frequency.

**Request Body:**
```json
{
  "centerFrequency": 3500000000
}
```

**Response:**
```json
{
  "success": true,
  "message": "Cell scan started",
  "centerFrequency": 3500000000,
  "cellsFound": 2,
  "cells": [
    {
      "id": 1,
      "pci": 124,
      "pss": { "detected": true, "value": 1 },
      "sss": { "detected": true, "value": 41 },
      "ssRsrp": -85,
      "ssRsrq": -12,
      "ssSinr": 15,
      "pbch": "Decoded",
      "sib1": "Detected",
      "mib": {
        "systemFrameNumber": 1234,
        "subCarrierSpacingCommon": 1,
        "ssbSubcarrierOffset": 0,
        "dmrsTypeAPosition": 2,
        "pdcchConfigSIB1": 0,
        "cellBarred": false,
        "intraFreqReselection": true,
        "spare": 0
      },
      "sib1Info": {
        "cellIdentity": 12345,
        "plmnIdentity": "001-01",
        "trackingAreaCode": 1,
        "cellReservedForOperatorUse": false,
        "cellSelectionInfo": {
          "qRxLevMin": -70,
          "qQualMin": -18,
          "qRxLevMinSUL": -70,
          "qQualMinSUL": -18
        },
        "freqBandIndicator": 78,
        "schedulingInfoList": [
          {
            "siBroadcastStatus": "broadcasting",
            "siPeriodicity": "rf16",
            "siRepetitionPattern": "every2ndRF"
          }
        ],
        "siWindowLength": "ms20",
        "systemInfoValueTag": 5,
        "lateNonCriticalExtension": false,
        "nonCriticalExtension": false
      }
    }
  ]
}
```

**POST** `/scan/stop`

Stop the current cell scan.

**Response:**
```json
{
  "success": true,
  "message": "Cell scan stopped"
}
```

**GET** `/scan/status`

Get the current cell scan status.

**Response:**
```json
{
  "isScanning": false,
  "centerFrequency": 3500000000,
  "detectedCells": [
    // Array of detected cells
  ]
}
```

**GET** `/scan/cells`

Get the list of detected cells.

**Response:**
```json
[
  {
    "id": 1,
    "pci": 124,
    "pss": { "detected": true, "value": 1 },
    "sss": { "detected": true, "value": 41 },
    // ... cell details
  }
]
```

### 5. UE Logs

**GET** `/logs`

Get UE process logs.

**Query Parameters:**
- `lines` (optional): Number of log lines to return (default: 100)

**Response:**
```json
{
  "processLogs": [
    {
      "timestamp": "2024-01-15T10:30:00.000Z",
      "type": "stdout",
      "message": "UE Process: Starting..."
    }
  ],
  "fileLogs": "Log content from file...",
  "logPath": "/tmp/ue.log",
  "lines": 100,
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

### 6. UE Metrics

**GET** `/metrics`

Get UE performance metrics.

**Response:**
```json
{
  "uptime": 30000,
  "status": "connected",
  "connected": true,
  "processId": 12345,
  "lastActivity": "2024-01-15T10:30:00.000Z",
  "error": null,
  "logEntries": 150,
  "cellsDetected": 2,
  "scanInProgress": false,
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

## Error Responses

All endpoints may return error responses in the following format:

```json
{
  "error": "Error message description"
}
```

**Common HTTP Status Codes:**
- `200`: Success
- `400`: Bad Request (invalid parameters)
- `404`: Not Found
- `500`: Internal Server Error

## Configuration Parameters

### UE Configuration Fields

| Field | Type | Description | Default |
|-------|------|-------------|---------|
| `imsi` | string | International Mobile Subscriber Identity | "001010000000003" |
| `key` | string | Authentication key | "fec86ba6eb707ed08905757b1bb44b8f" |
| `opc` | string | Operator variant algorithm configuration field | "c42449363bbad02b66d16bc975d77cc1" |
| `dnn` | string | Data Network Name | "oai" |
| `nssai_sst` | number | Network Slice Selection Assistance Information SST | 1 |
| `usrpArgs` | string | USRP device arguments | "type=x300,addr=192.168.40.2,clock=internal,time=internal" |
| `frequency` | number | Center frequency in Hz | 3425010000 |
| `numerology` | number | Subcarrier spacing numerology | 1 |
| `band` | number | 5G NR frequency band | 78 |

### Cell Information Fields

| Field | Type | Description |
|-------|------|-------------|
| `pci` | number | Physical Cell Identity (0-1007) |
| `pss` | object | Primary Synchronization Signal info |
| `sss` | object | Secondary Synchronization Signal info |
| `ssRsrp` | number | SS Reference Signal Received Power (dBm) |
| `ssRsrq` | number | SS Reference Signal Received Quality (dB) |
| `ssSinr` | number | SS Signal to Interference plus Noise Ratio (dB) |
| `pbch` | string | Physical Broadcast Channel status |
| `sib1` | string | System Information Block 1 status |
| `mib` | object | Master Information Block data |
| `sib1Info` | object | System Information Block 1 data |

## Usage Examples

### Start UE with custom frequency
```bash
curl -X POST http://localhost:40000/api/ue/start \
  -H "Content-Type: application/json" \
  -d '{"frequency": 3500000000, "band": 78}'
```

### Start cell scan
```bash
curl -X POST http://localhost:40000/api/ue/scan/start \
  -H "Content-Type: application/json" \
  -d '{"centerFrequency": 3500000000}'
```

### Get UE status
```bash
curl http://localhost:40000/api/ue/status
```

## Testing

Run the test script to verify all endpoints:

```bash
cd /home/fcp/openairinterface5g/webif/back-end
node test-ue-api.js
```

## Notes

- The UE process requires sudo privileges to run
- Cell scan functionality is currently simulated for demonstration purposes
- Real UE implementation would require integration with the actual OpenAirInterface5G UE softmodem
- The API supports both real-time process management and simulated data for development/testing
