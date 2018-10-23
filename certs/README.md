## Generate Attestation Key and Certificate

For more security, remember to generate a new attestation key and certificate.

Change to the `tools` directory, run the `generate-certs.sh` to generate a new attestation key and certificate:

``` sh
$ cd ./nrf52-u2f/tools

$ ./generate-certs.sh
```

If successfully completed, the private key and certificate are stored in `certs/keys.c` file.
