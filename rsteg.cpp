#include <iomanip>
#include <algorithm>
#include <chrono>
#include "io_helpers.hpp"
#include "lsb_rand.hpp"
#include "aes_helpers.hpp"

#ifdef _WIN32
    const unsigned int MIN = UINT32_MAX;
    const unsigned long long MAX = ULONG_MAX;
#else
    const unsigned int MIN = UINT16_MAX;
    // const unsigned long long MAX = ULONG_MAX;   // overflow on unix
    const unsigned int MAX = UINT32_MAX;
#endif

unsigned long long generateSeed(int numPositions) {

    std::random_device rd;
    std::uniform_int_distribution<unsigned long long> distribution(MIN, MAX);
    unsigned long long seedValue = distribution(rd);

    int size = static_cast<int>(log10(numPositions) + 1);

    std::stringstream seedStream;
    seedStream << seedValue << numPositions << size;

    unsigned long long combinedValue;
    seedStream >> combinedValue;

    std::cout << "using seed:   " << combinedValue << std::endl;

    return combinedValue;
}

// Parse args
bool parseArgs (int& argc, char** argv, std::vector<int>& index){

    std::vector<std::string> args;
    int i = 0;
    while(i < argc) {
        args.push_back(argv[i++]);
    }

    if (argc < 2) {
        std::cerr << "rsteg --help for usage instructions." << std::endl;
        return false;
    }

    else if(argc == 2 && (strcmp(argv[1], "--help") == 0)){
        std::cout << "Rsteg version 1.0\n";
        std::cout << "Written By Aqib Khan\n";
        std::cout << "This software is distributed under the MIT License\n\n";
        std::cout << "Available modes:\n";
        std::cout << "+-------+-------------------------------------------------------------------+\n";
        std::cout << "| Mode  | Description                                                       |\n";
        std::cout << "+-------+-------------------------------------------------------------------+\n";
        std::cout << "| enc   | encrypt file and embed in container                               |\n";
        std::cout << "| dec   | extract from container and decrypt files                          |\n";
        std::cout << "+------------------+--------------------------------------------------------+\n";
        std::cout << "| Symmetric Mode   | Description                                            |\n";
        std::cout << "+------------------+--------------------------------------------------------+\n";
        std::cout << "| AES-256 CBC      | Advanced Encryption Standard with 256-bit keys         |\n";
        std::cout << "|                  | in Cipher Block Chaining (CBC) mode.                   |\n";
        std::cout << "+------------------+--------------------------------------------------------+\n\n";
        std::cout << "Options:\n";
        std::cout << "+---------+-----------------------------------------------------------------+\n";
        std::cout << "| Option  | Description                                                     |\n";
        std::cout << "+---------+-----------------------------------------------------------------+\n";
        std::cout << "|  -i     | container file path                                             |\n";
        std::cout << "|         |     - input container path [ mode : enc ]                       |\n";
        std::cout << "|         |     - stego container path [ mode : dec ]                       |\n";
        std::cout << "|         |                                                                 |\n";
        std::cout << "|         | [ .PNG  .AVI ] supported containers                             |\n";
        std::cout << "|         |                                                                 |\n";
        std::cout << "|  -o     | output path [ optional ]                                        |\n";
        std::cout << "|         |     - default [ mode : enc ]  out.[ container extension ]       |\n";
        std::cout << "|         |     - default [ mode : dec ]  file.[ embed file extension ]     |\n";
        std::cout << "|         |                                                                 |\n";
        std::cout << "|  -m     | path to file                                                    |\n";
        std::cout << "|  -mk    | path to 256-bit AES message key file                            |\n";
        std::cout << "|  -sk    | path to 256-bit AES seed key file                               |\n";
        std::cout << "+---------+-----------------------------------------------------------------+\n";

        return false;
    }

    else if (strcmp(argv[1], "enc") == 0){
        if (argc < 10 || argc > 14) {
            std::cerr << "usage: rsteg enc\n" << std::endl;
            std::cerr << "          -i      [ container file ]" << std::endl;
            std::cerr << "          -m      [ embed file ]" << std::endl;
            std::cerr << "          -mk     [ message key file ]" << std::endl;
            std::cerr << "          -sk     [ seed key file ]" << std::endl;
            std::cerr << "OPTIONAL: -o      [ output image file ]\n" << std::endl;
            std::cerr << "rsteg --help for more information" << std::endl;

            return false;
        }

        index.push_back(std::find(args.begin(), args.end(), "-i") - args.begin());
        index.push_back(std::find(args.begin(), args.end(), "-o") != args.end() ?
                        (std::find(args.begin(), args.end(), "-o") - args.begin()) : -1);
        index.push_back(std::find(args.begin(), args.end(), "-m") - args.begin());
        index.push_back(std::find(args.begin(), args.end(), "-mk") - args.begin());
        index.push_back(std::find(args.begin(), args.end(), "-sk") - args.begin());
    }

    else if (strcmp(argv[1], "dec") == 0){
        if (argc < 8 || argc > 14) {
            std::cerr << "usage: rsteg dec\n" << std::endl;
            std::cerr << "          -i      [ container file ]" << std::endl;
            std::cerr << "          -mk     [ message key file ]" << std::endl;
            std::cerr << "          -sk     [ seed key file ]" << std::endl;
            std::cerr << "OPTIONAL: -o      [ output filename ]\n" << std::endl;
            std::cerr << "rsteg --help for more information" << std::endl;

            return false;
        }

        index.push_back(std::find(args.begin(), args.end(), "-i") - args.begin());
        index.push_back(std::find(args.begin(), args.end(), "-mk") - args.begin());
        index.push_back(std::find(args.begin(), args.end(), "-sk") - args.begin());
        index.push_back(std::find(args.begin(), args.end(), "-o") != args.end() ?
                        (std::find(args.begin(), args.end(), "-o") - args.begin()) : -1);
    }

    for (int i=0; i<static_cast<int>(index.size()); ++i){
        if (std::count(index.begin(), index.end(), index[i]) > 1 || index[i] == argc) {
            std::cerr << "invalid arguments ... " << std::endl << "rsteg --help for more details." << std::endl;
            return false;
        }
    }

    return true;
}

std::string getFileExtension(const std::vector<unsigned char>& data) {

    if (data[0] == 0x50 && data[1] == 0x4B && data[2] == 0x03 && data[3] == 0x04) {
        return ".zip";
    }
    else if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
        return ".jpg";
    }
    else if (data[0] == 0x52 && data[1] == 0x49 && data[2] == 0x46 && data[3] == 0x46) {
        return ".wav";
    }
    else if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
        return ".png";
    }
    else if (data[0] == 0x25 && data[1] == 0x50 && data[2] == 0x44 && data[3] == 0x46) {
        return ".pdf";
    }
    else if (data[0] == 0x47 && data[1] == 0x49 && data[2] == 0x46 && data[3] == 0x38) {
        return ".gif";
    }
    else if ((data[0] == 0x49 && data[1] == 0x44 && data[2] == 0x33) || 
             (data[0] == 0xFF && data[1] == 0xFB) ||
             (data[0] == 0xFF && data[1] == 0xF3)) {
        return ".mp3";
    }

    return ".txt";
}


int main(int argc, char** argv) {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    std::vector<int> index;
    if (!parseArgs(argc, argv, index)){
        return 1;
    }

    if (strcmp(argv[1], "enc") == 0) {

        const char* inputImagePath = argv[++index[0]];
        const char* outputImagePath = index[1] == -1 ? "." : argv[++index[1]];
        const char* inputFile = argv[++index[2]];
        const char* messageKeyFile = argv[++index[3]];
        const char* seedKeyFile = argv[++index[4]];

        std::cout << outputImagePath << std::endl;

        std::pair<std::vector<int>, std::vector<unsigned char>> image;
        int length = strlen(inputImagePath);
        if (length >= 4 && strcmp(inputImagePath + length - 4, ".avi") == 0) {
            image = readVideo(inputImagePath);
        } else {
            image = readImage(inputImagePath);
        }

        std::vector<unsigned char> fileContents;
        if(!readBinaryFile(inputFile, fileContents)){
            std::cerr << "Error:    unable to read embed file" << std::endl;
            return 1;
        }

        unsigned char messageKey[32];
        if(!readAes256KeyFromFile(messageKeyFile, messageKey, sizeof(messageKey))){
            return -1;
        }

        std::vector<unsigned char> encryptedBytes(fileContents.size());

        int encryptedByteslength = encrypt(fileContents, static_cast<int>(fileContents.size()), messageKey, messageKey, encryptedBytes);

        /*  //  debug snippet

        std::cout << "AES-256 encrypted bytes: " << std::endl;
        bool first = true;
        for (auto ch : encryptedBytes) {
            if (!first) {
                std::cout << ' ';
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(ch));
            first = false;
        }
        std::cout << std::dec << std::endl;  */

        // Calculate size for encoding
        int numPositions = (static_cast<long>(encryptedBytes.size()) * 8) / (4 * 2) * 4; // this works but how ??

        std::cout << std::fixed << std::setprecision(1) << "minimum required container size:   " << static_cast<double>(numPositions)/1024.0 << " KB" << std::endl; 

        if (numPositions > image.second.size()) {
            std::cerr << "Error:    insufficient container size" << std::endl;
            return 1;
        }

        std::cout << "file size:    " << std::fixed << std::setprecision(1) << static_cast<double>(encryptedBytes.size())/1024.0 << " KB" << std::endl;
        std::cout << "container size:   " << std::fixed << std::setprecision(1) << static_cast<double>(image.second.size())/1024.0 << " KB" << std::endl;

        unsigned char seedKey[32];
        if(!readAes256KeyFromFile(seedKeyFile, seedKey, 32)){
            return -1;
        }

        unsigned long long Seed = generateSeed(numPositions);
               
        if (Seed != 0) {
            unsigned char seedBytes[sizeof(Seed)];
            for (long long unsigned int i = 0; i < sizeof(Seed); ++i) {
                seedBytes[i] = (Seed >> (8 * i)) & 0xFF;
            }

            unsigned char encryptedSeed[AES_BLOCK_SIZE];
            int encryptedSeedLength = encrypt_seed(seedBytes, sizeof(seedBytes), seedKey, seedKey, encryptedSeed);

            std::cout << "AES-256 encrypted seed bytes:     ";
            for (int i = 0; i < encryptedSeedLength; ++i) {
                if (i != 0) {
                    std::cout << ' ';
                }
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(encryptedSeed[i]);
            }
            std::cout << std::dec << std::endl;

            auto start = std::chrono::high_resolution_clock::now();
            std::vector<int> positions = generateRandomPositions(image.second, Seed);
            auto stop = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
            std::cout << "generated encoding sequence in " << std::setprecision(2) << static_cast<double>(duration.count()) << " s" << std::endl; 

            // works now
            encode_lsb(image.second, encryptedBytes, positions);

            std::vector<unsigned char> encodedSeedBytes;
            for (int i = 0; i < encryptedSeedLength; ++i) {
                encodedSeedBytes.push_back(encryptedSeed[i]);
            }
            encodedSeedBytes.push_back(encryptedSeedLength);

            length = strlen(inputImagePath);
            if (length >= 4 && strcmp(inputImagePath + length - 4, ".avi") == 0 && strcmp(outputImagePath, ".") == 0) {
                outputImagePath = "./out.avi";
                if(!writeVideo(outputImagePath, image.second, image.first[0], image.first[1], image.first[2])) {
                    std::cerr << "Error:    failed to write to container" << std::endl;
                    return 1;
                }
            } else if (length >= 4 && strcmp(inputImagePath + length - 4, ".avi") == 0 && strcmp(outputImagePath, ".") != 0){
                if(!writeImage(outputImagePath, image.second, image.first[0], image.first[1], image.first[2])) {
                    std::cerr << "Error:    failed to write to container" << std::endl;
                    return 1;
                }
            } else {
                outputImagePath = "./out.png";
                if(!writeImage(outputImagePath, image.second, image.first[0], image.first[1], image.first[2])) {
                    std::cerr << "Error:    failed to write to container" << std::endl;
                    return 1;
                }
            }

            // write the seed
            std::ofstream outputFile(outputImagePath, std::ios::out | std::ios::app | std::ios::binary);
            if (outputFile.is_open()) {
                outputFile.write(reinterpret_cast<char*>(encodedSeedBytes.data()), encodedSeedBytes.size());
                outputFile.close();
                std::cout << "seed written to container." << std::endl;
            } else {
                std::cerr << "Error:    failed to embed seed bytes." << std::endl;
            }

            std::cout << "successfully created embedded container:      " << outputImagePath << std::endl;

        } else {
            std::cerr << "Error:    Unknown" << std::endl;
            return 1;
        }

    } else if (strcmp(argv[1], "dec") == 0) {

        const char* inputImagePath = argv[++index[0]];
        const char* messageKeyFile = argv[++index[1]];
        const char* seedKeyFile = argv[++index[2]];
        std::string outputFilename = index[3] == -1 ? "." : argv[++index[3]];
        
        std::vector<unsigned char> encryptedSeed = decodeSeedBytes(inputImagePath);

        size_t i = encryptedSeed.size()-1;

        // remove any padding
        while(true) {
            if(encryptedSeed[i] == 0x00) {
                encryptedSeed.pop_back();
            } else {
                break;
            }
            --i;
        }

        unsigned char seedKey[32];
        if(!readAes256KeyFromFile(seedKeyFile, seedKey, 32)){
            return -1;
        }

        std::cout << "extracted seed:   ";
        for (size_t i = 0; i < encryptedSeed.size(); ++i) {
            if (i != 0) {
                std::cout << ' ';
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(encryptedSeed[i]);
        }
        std::cout << std::dec << std::endl;

        unsigned long long decryptedSeed = 0;
        if(decrypt_seed(encryptedSeed.data(), static_cast<int>(encryptedSeed.size()), seedKey, seedKey, reinterpret_cast<unsigned char*>(&decryptedSeed)) < 0) {
            std::cerr << "Error:    failed to decrypt seed" << std::endl;
            return 1;
        }

        std::pair<std::vector<int>, std::vector<unsigned char>> stegoImage;

        int length = strlen(inputImagePath);
        if (length >= 4 && strcmp(inputImagePath + length - 4, ".avi") == 0) {
            stegoImage = readVideo(inputImagePath);
        } else {
            stegoImage = readImage(inputImagePath);
        }

        std::cout << "decrypted seed:   " << decryptedSeed << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        std::vector<int> positions = generateRandomPositions(stegoImage.second, decryptedSeed);
        auto stop = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
        std::cout << "generated encoding sequence in " << std::setprecision(2) << static_cast<double>(duration.count()) << " s" << std::endl; 

        std::vector<unsigned char> extractedBytes = decode_file(stegoImage.second, positions);

        /*  // debug snippet

        std::cout << "extracted bytes: " << std::endl;
        bool first = true;
        for (auto ch : extractedBytes) {
            if (!first) {
                std::cout << ' '; // Add a space separator
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(ch));
            first = false;
        }
        std::cout << std::dec << std::endl;  */

        i = extractedBytes.size()-1;

        // remove any padding
        while(true) {
            if(extractedBytes[i] == 0x00) {
                extractedBytes.pop_back();
            } else {
                break;
            }
            --i;
        }        

        unsigned char messageKey[32];
        if(!readAes256KeyFromFile(messageKeyFile, messageKey, sizeof(messageKey))){
            return -1;
        }

        std::vector<unsigned char> finalMessageBytes(extractedBytes.size());

        if(decrypt(extractedBytes, static_cast<int>(extractedBytes.size()), messageKey, messageKey, finalMessageBytes) < 0) {
            std::cerr << "Error:    unable to decrypt extracted file" << std::endl;
            return 1;
        }

        std::vector<unsigned char> slicedData(finalMessageBytes.begin(), finalMessageBytes.begin() + 4);

        std::string ext = getFileExtension(slicedData);

        std::ofstream outputFile(outputFilename+ext, std::ios::binary);

        i = finalMessageBytes.size()-1;

        // remove any padding
        while(true) {
            if(finalMessageBytes[i] == 0x00) {
                finalMessageBytes.pop_back();
            } else {
                break;
            }
            --i;
        }

        if (outputFile.is_open()) {
            outputFile.write(reinterpret_cast<const char*>(finalMessageBytes.data()), finalMessageBytes.size());
            outputFile.close();
            std::cout << "reconstructed the file:   " << outputFilename+ext << std::endl;
        } else {
            return false;
        }

    } else {
        std::cerr << "rsteg --help for more information" << std::endl;
        return 1;
    }

    return 0;
}
