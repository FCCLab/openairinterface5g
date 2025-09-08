#!/bin/bash

# Generate protobuf files for JavaScript and Python

echo "Generating protobuf files..."

# Create directories if they don't exist
mkdir -p back-end/proto
mkdir -p ue/proto

# Generate JavaScript protobuf files (optional - backend uses dynamic loading)
echo "Skipping JavaScript protobuf generation (backend uses dynamic loading)..."
# npx grpc_tools_node_protoc \
#   --js_out=import_style=commonjs,binary:back-end/proto \
#   --grpc_out=grpc_js:back-end/proto \
#   --plugin=protoc-gen-grpc=./node_modules/.bin/grpc_tools_node_protoc_plugin \
#   proto/ue_service.proto

# Generate Python protobuf files
echo "Generating Python protobuf files..."
python3 -m grpc_tools.protoc \
  --python_out=ue/proto \
  --grpc_python_out=ue/proto \
  --proto_path=proto \
  proto/ue_service.proto

echo "Protobuf files generated successfully!"
echo "JavaScript files: back-end/proto/"
echo "Python files: ue/proto/"
