#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>
#include <opencv2/opencv.hpp>

extern "C" {
    #include <png.h>
}

bool readAes256KeyFromFile(const char* fileName, unsigned char* key, int keySize) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Error:    unable to read key file" << fileName << std::endl;
        return false;
    }

    std::string keyHex;
    file >> keyHex;

    if (keyHex.length() != static_cast<std::string::size_type>(2 * keySize)) {
        std::cerr << "Error:    256-bit keys only" << fileName << std::endl;
        return false;
    }

    for (int i = 0; i < keySize; ++i) {
        std::istringstream iss(keyHex.substr(2 * i, 2));
        int byteValue;
        if (!(iss >> std::hex >> byteValue)) {
            std::cerr << "Error:    invalid key format" << fileName << std::endl;
            return false;
        }
        key[i] = static_cast<unsigned char>(byteValue);
    }

    return true;
}

// update for linux IO
bool readBinaryFile(const char* filename, std::vector<unsigned char>& data) {
    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "Error:    unable to open the file" << std::endl;
        return false;
    }

    inputFile.seekg(0, std::ios::end);
    std::streamsize fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (fileSize <= 0) {
        std::cerr << "Error:    no data to read" << std::endl;
        return false;
    }

    data.resize(static_cast<size_t>(fileSize));
    inputFile.read(reinterpret_cast<char*>(data.data()), fileSize);

    inputFile.close();

    return true;
}

std::pair<std::vector<int>, std::vector<unsigned char>> readImage(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error:     unable to read PNG file\n");
        exit(1);
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        fprintf(stderr, "png_create_read_struct failed.\n");
        exit(1);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fclose(fp);
        png_destroy_read_struct(&png, NULL, NULL);
        fprintf(stderr, "png_create_info_struct failed.\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf(png))) {
        fclose(fp);
        png_destroy_read_struct(&png, &info, NULL);
        fprintf(stderr, "Error during png_init_io or png_read_info.\n");
        exit(1);
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    int num_channels = (color_type == PNG_COLOR_TYPE_RGB) ? 3 : 4;

    std::vector<unsigned char> imageData;
    png_bytep row = (png_bytep)malloc(num_channels * width);

    for (int y = 0; y < height; y++) {
        png_read_row(png, row, NULL);
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < num_channels; c++) {
                imageData.push_back(row[x * num_channels + c]);
            }
        }
    }

    free(row);
    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);

    return std::make_pair(std::vector<int>{width, height, num_channels}, imageData);
}

bool writeImage(const char* filename, const std::vector<unsigned char>& imageData, int width, int height, int numChannels) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error:     failed to create output PNG\n");
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        fprintf(stderr, "png_create_write_struct failed.\n");
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fclose(fp);
        png_destroy_write_struct(&png, (png_infopp)NULL);
        fprintf(stderr, "png_create_info_struct failed.\n");
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        fprintf(stderr, "Error during png_init_io or png_write_info.\n");
        return false;
    }

    png_init_io(png, fp);

    png_byte color_type;
    if (numChannels == 3) {
        color_type = PNG_COLOR_TYPE_RGB;
    } else if (numChannels == 4) {
        color_type = PNG_COLOR_TYPE_RGBA;
    } else {
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        fprintf(stderr, "Error:     unsupported number of channels.\n");
        return false;
    }

    png_set_IHDR(png, info, width, height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    png_set_compression_level(png, 0);
    png_set_compression_strategy(png, 0);
    png_set_filter(png, 0, PNG_FILTER_NONE);

    std::vector<png_byte> row(numChannels * width);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < numChannels * width; x++) {
            row[x] = imageData[y * numChannels * width + x];
        }
        png_write_row(png, row.data());
    }

    png_write_end(png, NULL);

    fclose(fp);
    png_destroy_write_struct(&png, &info);

    return true;
}

// extract seed
std::vector<unsigned char> decodeSeedBytes(const std::string& filePath) {
    std::vector<unsigned char> decodedSeedBytes;
    
    std::ifstream inputFile(filePath, std::ios::binary);
    
    if (!inputFile.is_open()) {
        std::cerr << "Error: unable to read seed" << std::endl;
        exit(1);
    }
    
    inputFile.seekg(-1, std::ios::end);
    int seedLength = 0;
    inputFile.read(reinterpret_cast<char*>(&seedLength), 1);

    if (seedLength < 0) {
        std::cerr << "Error: invalid seed length" << std::endl;
        exit(1);
    }
    
    decodedSeedBytes.resize(seedLength);

    inputFile.seekg(-seedLength-1, std::ios::cur);
    inputFile.read(reinterpret_cast<char*>(&decodedSeedBytes[0]), seedLength);
    
    inputFile.close();
    
    return decodedSeedBytes;
}

std::pair<std::vector<int>, std::vector<unsigned char>> readVideo(const char* videoFileName) {
    cv::VideoCapture cap(videoFileName);

    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the video file." << std::endl;
        exit(1);
    }

    std::cout << "Reading video file..." << std::endl;

    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int numChannels = 0;

    cv::Mat frame;

    std::vector<unsigned char> bytes;

    while (true) {
        cap >> frame; // Read a frame

        if (frame.empty()) {
            break;
        }

        numChannels = frame.channels(); // Get the number of color channels

        // Iterate through each pixel in the frame
        for (int y = 0; y < frame.rows; ++y) {
            for (int x = 0; x < frame.cols; ++x) {
                for (int c = 0; c < numChannels; ++c) {
                    bytes.push_back(frame.at<uchar>(y, x * numChannels + c));
                }
            }
        }
    }

    cap.release();

    return std::make_pair(std::vector<int>{width, height, numChannels}, bytes);
}

bool writeVideo(const char* videoFileName, const std::vector<unsigned char>& bytes, int width, int height, int numChannels) {
    cv::VideoWriter writer(videoFileName, cv::VideoWriter::fourcc('F','F','V','1'), 30, cv::Size(width, height), true);

    if (!writer.isOpened()) {
        std::cerr << "Error: Could not open the VideoWriter." << std::endl;
        return false;
    }

    // Create a frame with the specified number of channels
    cv::Mat frame(height, width, CV_8UC(numChannels));

    // Ensure the byteIndex stays within bounds
    unsigned long long byteIndex = 0;
    unsigned long long totalBytes = static_cast<unsigned long long>(bytes.size());

    while (byteIndex < totalBytes) {
        for (int y = 0; y < frame.rows; ++y) {
            for (int x = 0; x < frame.cols; ++x) {
                for (int c = 0; c < numChannels; ++c) {
                    if (byteIndex < totalBytes) {
                        frame.at<uchar>(y, x * numChannels + c) = bytes[byteIndex]; // Set each channel
                        byteIndex++;
                    }
                }
            }
        }

        writer.write(frame); // Write the frame to the video
    }

    writer.release();
    
    return true;
}
