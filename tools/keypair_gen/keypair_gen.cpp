// keypair_gen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "CryptoPP/eccrypto.h"
#include "CryptoPP/sha.h"
#include "CryptoPP/oids.h"
#include "CryptoPP/osrng.h"
#include "CryptoPP/files.h"
#include "CryptoPP/base64.h"

void generate_ecdsa_keypair()
{
	using D = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>;

	CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP> gp;
	gp.Initialize(CryptoPP::ASN1::secp384r1());

	CryptoPP::AutoSeededRandomPool rand_pool;

	D::PrivateKey priv_key;
	priv_key.GenerateRandom(rand_pool, gp);

	D::PublicKey pub_key;
	priv_key.MakePublicKey(pub_key);

	std::string priv_key_der, public_key_der;

	// Encode private key in base64
	{
		CryptoPP::Base64Encoder b64{ new CryptoPP::StringSink{ priv_key_der } };
		priv_key.BEREncode(b64);
		b64.MessageEnd();
	}

	// Encode public key in base64
	{
		CryptoPP::Base64Encoder b64{ new CryptoPP::StringSink{ public_key_der } };
		pub_key.BEREncode(b64);
		b64.MessageEnd();
	}

	std::cout << "Private Key:\n" << priv_key_der << "\n";
	std::cout << "Public Key:\n" << public_key_der << "\n";
}

int main()
{
	generate_ecdsa_keypair();
	return 0;
}

