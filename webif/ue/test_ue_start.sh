#!/bin/bash

# Test script to start UE with configuration parameters
curl -X POST -H "Content-Type: application/json" \
-d '{
  "frequency": 3425010000,
  "authentication": {
    "imsi": "001010000000003",
    "key": "fec86ba6eb707ed08905757b1bb44b8f",
    "opc": "c42449363bbad02b66d16bc975d77cc1"
  },
  "network": {
    "dnn": "oai",
    "nssai_sst": "1"
  },
  "radio": {
    "bandwidth": 100,
    "numerology": 1
  }
}' \
http://10.1.100.143:40000/api/ue/start | jq .
