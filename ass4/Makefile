CC=clang
CFLAGS=-Wall -Wall -Wextra -g -std=c99 -DDEBUG=1

all: keygen.c otp_enc_d otp_dec_d otp_enc otp_dec

keygen.c:
	${CC} keygen.c -o keygen ${CFLAGS}

otp_enc_d:
	${CC} server.c -o otp_enc_d -DENCRYPT=1 ${CFLAGS}

otp_dec_d:
	${CC} server.c -o otp_dec_d -DDECRYPT=1 ${CFLAGS}

otp_enc:
	${CC} client.c -o otp_dec -DDECRYPT=1 ${CFLAGS}

otp_dec:
	${CC} client.c -o otp_dec -DDECRYPT=1 ${CFLAGS}

.PHONY: clean
clean:
	rm -rf *.dSYM
	rm -f keygen
	rm -f otp_enc
	rm -f otp_dec
	rm -f otp_enc_d
	rm -f otp_dec_d
