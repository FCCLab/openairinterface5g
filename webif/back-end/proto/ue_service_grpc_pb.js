// GENERATED CODE -- DO NOT EDIT!

'use strict';
var grpc = require('@grpc/grpc-js');
var ue_service_pb = require('./ue_service_pb.js');

function serialize_ue_service_ConfigureUERequest(arg) {
  if (!(arg instanceof ue_service_pb.ConfigureUERequest)) {
    throw new Error('Expected argument of type ue_service.ConfigureUERequest');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_ConfigureUERequest(buffer_arg) {
  return ue_service_pb.ConfigureUERequest.deserializeBinary(new Uint8Array(buffer_arg));
}

function serialize_ue_service_ConfigureUEResponse(arg) {
  if (!(arg instanceof ue_service_pb.ConfigureUEResponse)) {
    throw new Error('Expected argument of type ue_service.ConfigureUEResponse');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_ConfigureUEResponse(buffer_arg) {
  return ue_service_pb.ConfigureUEResponse.deserializeBinary(new Uint8Array(buffer_arg));
}

function serialize_ue_service_DetectedCellRequest(arg) {
  if (!(arg instanceof ue_service_pb.DetectedCellRequest)) {
    throw new Error('Expected argument of type ue_service.DetectedCellRequest');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_DetectedCellRequest(buffer_arg) {
  return ue_service_pb.DetectedCellRequest.deserializeBinary(new Uint8Array(buffer_arg));
}

function serialize_ue_service_DetectedCellResponse(arg) {
  if (!(arg instanceof ue_service_pb.DetectedCellResponse)) {
    throw new Error('Expected argument of type ue_service.DetectedCellResponse');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_DetectedCellResponse(buffer_arg) {
  return ue_service_pb.DetectedCellResponse.deserializeBinary(new Uint8Array(buffer_arg));
}

function serialize_ue_service_StartScanningRequest(arg) {
  if (!(arg instanceof ue_service_pb.StartScanningRequest)) {
    throw new Error('Expected argument of type ue_service.StartScanningRequest');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_StartScanningRequest(buffer_arg) {
  return ue_service_pb.StartScanningRequest.deserializeBinary(new Uint8Array(buffer_arg));
}

function serialize_ue_service_StartScanningResponse(arg) {
  if (!(arg instanceof ue_service_pb.StartScanningResponse)) {
    throw new Error('Expected argument of type ue_service.StartScanningResponse');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_StartScanningResponse(buffer_arg) {
  return ue_service_pb.StartScanningResponse.deserializeBinary(new Uint8Array(buffer_arg));
}

function serialize_ue_service_StopScanningRequest(arg) {
  if (!(arg instanceof ue_service_pb.StopScanningRequest)) {
    throw new Error('Expected argument of type ue_service.StopScanningRequest');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_StopScanningRequest(buffer_arg) {
  return ue_service_pb.StopScanningRequest.deserializeBinary(new Uint8Array(buffer_arg));
}

function serialize_ue_service_StopScanningResponse(arg) {
  if (!(arg instanceof ue_service_pb.StopScanningResponse)) {
    throw new Error('Expected argument of type ue_service.StopScanningResponse');
  }
  return Buffer.from(arg.serializeBinary());
}

function deserialize_ue_service_StopScanningResponse(buffer_arg) {
  return ue_service_pb.StopScanningResponse.deserializeBinary(new Uint8Array(buffer_arg));
}


// UE Control Service
var UEControlServiceService = exports.UEControlServiceService = {
  // Configure UE parameters
configureUE: {
    path: '/ue_service.UEControlService/ConfigureUE',
    requestStream: false,
    responseStream: false,
    requestType: ue_service_pb.ConfigureUERequest,
    responseType: ue_service_pb.ConfigureUEResponse,
    requestSerialize: serialize_ue_service_ConfigureUERequest,
    requestDeserialize: deserialize_ue_service_ConfigureUERequest,
    responseSerialize: serialize_ue_service_ConfigureUEResponse,
    responseDeserialize: deserialize_ue_service_ConfigureUEResponse,
  },
  // Start cell scanning procedure
startScanning: {
    path: '/ue_service.UEControlService/StartScanning',
    requestStream: false,
    responseStream: false,
    requestType: ue_service_pb.StartScanningRequest,
    responseType: ue_service_pb.StartScanningResponse,
    requestSerialize: serialize_ue_service_StartScanningRequest,
    requestDeserialize: deserialize_ue_service_StartScanningRequest,
    responseSerialize: serialize_ue_service_StartScanningResponse,
    responseDeserialize: deserialize_ue_service_StartScanningResponse,
  },
  // Stop cell scanning procedure
stopScanning: {
    path: '/ue_service.UEControlService/StopScanning',
    requestStream: false,
    responseStream: false,
    requestType: ue_service_pb.StopScanningRequest,
    responseType: ue_service_pb.StopScanningResponse,
    requestSerialize: serialize_ue_service_StopScanningRequest,
    requestDeserialize: deserialize_ue_service_StopScanningRequest,
    responseSerialize: serialize_ue_service_StopScanningResponse,
    responseDeserialize: deserialize_ue_service_StopScanningResponse,
  },
};

exports.UEControlServiceClient = grpc.makeGenericClientConstructor(UEControlServiceService, 'UEControlService');
// UE Data Acquisition Service
var UEDataServiceService = exports.UEDataServiceService = {
  // Get detected cells
getDetectedCells: {
    path: '/ue_service.UEDataService/GetDetectedCells',
    requestStream: false,
    responseStream: false,
    requestType: ue_service_pb.DetectedCellRequest,
    responseType: ue_service_pb.DetectedCellResponse,
    requestSerialize: serialize_ue_service_DetectedCellRequest,
    requestDeserialize: deserialize_ue_service_DetectedCellRequest,
    responseSerialize: serialize_ue_service_DetectedCellResponse,
    responseDeserialize: deserialize_ue_service_DetectedCellResponse,
  },
};

exports.UEDataServiceClient = grpc.makeGenericClientConstructor(UEDataServiceService, 'UEDataService');
