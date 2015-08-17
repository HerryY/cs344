#!/bin/bash

gcc keygen.c -o keygen -std=c99
gcc server.c -o otp_enc_d -DENCRYPT=1 -std=c99
gcc server.c -o otp_dec_d -DDECRYPT=1 -std=c99
gcc client.c -o otp_enc -DENCRYPT=1 -std=c99
gcc client.c -o otp_dec -DDECRYPT=1 -std=c99
