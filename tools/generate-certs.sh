#!/bin/bash

# Copyright (c) 2018 makerdiary
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the following
#   disclaimer in the documentation and/or other materials provided
#   with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



set -e
set -o pipefail

# Check prerequisites
for command in openssl python; do
  if ! which "$command" >/dev/null 2>&1; then
    echo "Not find command: $command" >&2
    echo "Please install the corresponding package." >&2
    exit 1
  fi
done
for openssl_subcommand in ecparam req x509; do
  openssl help &> /tmp/.generate_certs
  if ! grep -q $openssl_subcommand /tmp/.generate_certs; then
    echo "OpenSSL does not support the \"$openssl_subcommand\" command." >&2
    echo "Please compile a full-featured version of OpenSSL." >&2
    rm -f /tmp/.generate_certs
    exit 1
  fi
done

# Change to certs directory
workdir="../certs"
cd "$workdir"

# Generate CA key and certificate
openssl ecparam -genkey -name prime256v1 -out ca.key
openssl req -config myserver.cnf -x509 -new -batch -SHA256 -nodes -key ca.key -days 3650 -out ca.crt

# Generate attestation key
openssl ecparam -genkey -name prime256v1 -out attestation.key

# Sign the attestation key with the certificate
openssl req -config myserver.cnf -new -batch -SHA256 -key attestation.key -nodes -out attestation.csr
openssl x509 -req -SHA256 -days 3650 -in attestation.csr -CA ca.crt -CAkey ca.key -CAcreateserial -outform DER -out attestation.der 2>/dev/null

# Print private key.
openssl ec -in attestation.key -noout -text

# Print certificate
openssl x509 -in attestation.der -inform DER -noout -text

python ../tools/cert2array.py attestation.key attestation.der


