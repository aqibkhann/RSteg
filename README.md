# Rsteg

Rsteg is a steganography tool based on the Least Significant Bit (LSB) technique, which enables embedding of files within image and video container formats.

## Features

- Currently supports .PNG and .AVI containers.

- File formats supported:  .zip .jpg/.jpeg .png .pdf .wav .mp3 .txt

- **Seed-Based Distribution**: The distribution of encoded data is determined using a seed value and encoded in random color channels. The generator implemented uses Mersenne Twister (PRNG).

- **Dual-Key AES-256 Encryption**: Embedded data and the seed is encrypted with seperate keys using AES-256 in Cipher Block Chaning mode. Distinct keys adds an extra layer as the first key decrypts the seed which is required to extract the embedded bytes in order else decryption of the file fails.

## Dependencies

- openssl
- libpng (>=1.6.37)
- openCV (4.8.0)

**Build using cmake**
- clone the repository<br>
```
git clone https://github.com/aqibkhann/rsteg.git
```
- navigate to the directory
- generate build files
```
mkdir build
cd build
cmake ..
``` 
  - on unix-based systems ```make```
  - on windows systems ```ninja``` 


## Usage:

- overview
```
./rsteg --help
```
- encode files
```
./rsteg enc -i [container file] -m [embed file] -mk [message key file] -sk [seed key file]
```
- decode containers
```
./rsteg dec -i [container file] -mk [message key file] -sk [seed key file]
```
