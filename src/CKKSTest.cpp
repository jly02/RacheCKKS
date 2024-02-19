#include <iostream>
#include "seal/seal.h"
#include "racheaan.h"

using namespace std;
using namespace seal;
using namespace racheaan;

void initialize(int arr[], int size);

// size of random array to benchmark
const int SIZE = 20;

// number of initial ciphertexts to be cached
const int INIT_CACHE_SIZE = 15;

// minimum size of values to be benchmarked
// Inv: MIN_VAL > 0
const int MIN_VAL = 1;

// maximum size of values to be benchmarked
// If n = INIT_CACHE_SIZE, then should have something like MAX_VAL < 2^n
const int MAX_VAL = 30000;

/**
 * Some benchmarks to test performance differences.
 */
int main()
{
    // set up params
    EncryptionParameters params(scheme_type::ckks);
    size_t poly_modulus_degree = 8192;
    params.set_poly_modulus_degree(poly_modulus_degree);

    // choose 60 bit primes for first and last (last should just be at least as large as first)
    // also choose intermediate primes to be close to each other
    params.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 60, 40, 40, 60 }));

    // scale stabilization with 2^40 scale, close to the intermediate primes
    double scale = pow(2.0, 40);

    // context gathers params
    SEALContext context(params);

    // generate keys
    KeyGenerator keygen(context);
    SecretKey secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);
   
    // encryptor, evalutator, and decryptor
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);
    Decryptor decryptor(context, secret_key);

    // encoder for ckks scheme
    CKKSEncoder encoder(context);

    // array of random integers to be encoded
    cout << "Generating random array of integers..." << endl;
    int random_arr[SIZE];
    initialize(random_arr, SIZE);

    cout << "Encrypting random array with pure CKKS..." << endl;

    Plaintext plain;
    Ciphertext cipher;
    auto start = chrono::high_resolution_clock::now();
    // encode and encrypt small batch of numbers
    for (int i = 0; i < SIZE; i ++) {
        encoder.encode(random_arr[i], scale, plain);
        encryptor.encrypt(plain, cipher);
    }
    // timing this small test
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Encryption of " << SIZE << " numbers in CKKS took " << duration.count() << " milliseconds." << endl;

    // Rache timing
    cout << "Testing same array with Rache..." << endl;

    // timing initialization
    start = chrono::high_resolution_clock::now();
    Rache rache(INIT_CACHE_SIZE);
    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Initialization of cache took " << duration.count() << " milliseconds." << endl;
    
    // Store ciphertexts to check output later
    Ciphertext ctxt[SIZE];

    cout << "Encrypting random array with Rache..." << endl;
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < SIZE; i ++) {
        rache.encrypt(random_arr[i], ctxt[i]);
    }
    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
    cout << "Encryption of " << SIZE << " numbers in Rache took " << duration.count() << " milliseconds." << endl;

    // print decrypted ciphertexts
    vector<double> output(SIZE);
    for (int i = 0; i < SIZE; i++) {
        Plaintext rache_plain;
        rache.decrypt(ctxt[i], rache_plain);
        vector<double> rache_decoded;
        encoder.decode(rache_plain, rache_decoded);
        output[i] = rache_decoded[0];
    }

    for (int i = 0; i < SIZE; i++) {
        cout << output[i] << " ";
    }

    cout << endl;

    return 0;
}

// initializes an array with random values
void initialize(int arr[], int size) {
    srand(time(0));

    for(int i = 0; i < size; i++){
        arr[i] = (rand() % MAX_VAL) + MIN_VAL;
    }

    for(int i = 0; i < size; i++){
        cout << arr[i] << " ";
    }

    cout << endl;
}

