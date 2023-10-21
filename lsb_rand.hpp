#include <random>
#include <bitset>

void encode_lsb(std::vector<unsigned char>& imageData, std::vector<unsigned char>& fileData, std::vector<int>& positions) {
	int b = 0;
    int c = 1;
    int shift = 6;
    bool err = false;

    std::vector<unsigned char>copyofData = fileData;
    std::vector<unsigned char>checkbits;
    unsigned char currentByte = 0x00;

    std::cout << "encoding file ..." << std::endl;

	for (auto position : positions)
	{
		unsigned char val = imageData[position];

        if(b>=fileData.size()) {
            std::cerr << "Error:    past eof error" << std::endl;
            break;
        }

		if (c % 5 == 0) {
			for (auto checkbit : checkbits) {
				unsigned char tmp1 = checkbit;
				currentByte |= ((tmp1 & 0x03) << shift);
				shift -= 2;
			}

            std::bitset<16> x(currentByte);
            std::bitset<16> y(copyofData[b]);

            if(currentByte != copyofData[b]) {
                std::cout << "Error:    encoding expected:  " << x << "     got: " << y << std::endl;
                err = true;
            }

            ++b;
            c = 1;
            shift = 6;
            checkbits.clear();
            currentByte = 0x00;
		}

		val &= 0xFC;
		auto tmp = fileData[b];
		val |= ((tmp & 0xc0) >> 6);

        unsigned char dataToWrite = static_cast<unsigned char>((tmp & 0xc0) >> 6);

		checkbits.push_back(val);

		fileData[b] <<= 2;

		++c;
            
		imageData[position] = val;
 	}
}

std::vector<unsigned char> decode_file(std::vector<unsigned char>& imageFile, std::vector<int>& positions) {

    std::cout << "decoding file ..." << std::endl;

    std::vector<unsigned char> data;
    unsigned char currentByte = 0x00;
    int shift = 6; int b = 0; int n = 0; int corrupt = 0;

    for (auto position : positions) {

        unsigned char val = imageFile[position];
            
        unsigned char tmp = val;

        currentByte |= ((tmp & 0x03) << shift);

        shift -= 2;
        ++n;

        if (shift < 0) {
            data.push_back(currentByte);
            currentByte = 0x00;
            shift = 6;
        }
    }

    return data;
}

// O(n) using std::shuffle
std::vector<int> generateRandomPositions(std::vector<unsigned char> image, unsigned long long seed) {
    std::vector<int> positions;

    if (seed == 0) {
        std::cerr << "Error:    bad seed" << std::endl;
        exit(1);
    }

    std::cout << "generating randomized embed order from seed ..." << std::endl;

    int numPositions = 0;
    int positionsLength = seed % 10;
    seed /= 10;
    for (int i = 0; i < positionsLength; ++i) {
        numPositions += (seed % 10) * static_cast<int>(pow(10, i));
        seed /= 10;
    }

    std::vector<int> allPositions(numPositions);
    for (int i = 0; i < numPositions; ++i) {
        allPositions[i] = i;
    }

    std::mt19937 gen(static_cast<unsigned long long>(seed));

    std::shuffle(allPositions.begin(), allPositions.end(), gen);

    positions.insert(positions.begin(), allPositions.begin(), allPositions.begin() + numPositions);

    return positions;
}
