//
// Tekken 3 BinStream Extractor & Packer
// Extracts the TEKKEN3.BNS file
//
// by Xan / Tenjoin
//

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "include/patterns.hpp"

#define VAB_HEADER_MAGIC 0x56414270
#define MODEL_HEADER_MAGIC 0x4B4D4433

size_t alignTo0x800(size_t offset)
{
    constexpr size_t alignment = 0x800;

    size_t remainder = offset % alignment;

    if (remainder != 0) {
        offset += (alignment - remainder);
    }

    return offset;
}

bool numericFilenameComparator(const std::filesystem::path& path1, const std::filesystem::path& path2)
{
    int numericPart1 = std::stoi(path1.stem().string());
    int numericPart2 = std::stoi(path2.stem().string());

    return numericPart1 < numericPart2;
}


bool isNumericFilename(const std::filesystem::path& path)
{
    std::string filename = path.stem().string();
	return std::all_of(filename.begin(), filename.end(), [](char c)
		{
			return std::isdigit(static_cast<unsigned char>(c));
		});
}

int FindLBATable(std::filesystem::path ExePath, uintptr_t* outPos, uint32_t* outCount)
{
    constexpr uintptr_t psxMemBase = 0x80000000;
    constexpr uintptr_t psxTextBase = 0x80010000;
    constexpr uintptr_t psxExeTextStart = 0x800;

    std::ifstream exefile;

    exefile.open(ExePath, std::ios::binary);
    if (!exefile.is_open())
    {
        char errstr[512];
        std::cerr << "ERROR: " << "Can't open exe file " << ExePath.string() << " for reading! Reason: " << strerror_s(errstr, errno) << '\n';
        return -1;
    }

    uintmax_t exesize = std::filesystem::file_size(ExePath);
    char* exeBuffer = (char*)malloc(exesize);
    exefile.read(exeBuffer, exesize);
    exefile.close();

    pattern::SetGameBaseAddress((uintptr_t)&exeBuffer[psxExeTextStart], exesize - psxExeTextStart);

    // addresses are based off of the JP build
    uintptr_t loc_8006C7E4 = pattern::get_first("07 00 02 92 00 00 00 00 FD FF 40 14 ? ? 04 3C", 0);
    if (!loc_8006C7E4)
        return -2;

    // look for the LBA table, in JP build it's at 0x8002466C
    // lui $a0, 0x8002
    uintptr_t loc_8006C7F0 = loc_8006C7E4 + 0xC;
    // ori $a0, $a0, 0x466C
    uintptr_t loc_8006C808 = loc_8006C7E4 + 0x24;

    // since the R3000 doesn't require any special babysitting of the sign bit overflowing into the upper
    // half of the register, we can simply read values as-is
    uintptr_t lbaTable = (((*(uint32_t*)loc_8006C7F0) & 0xFFFF) << 16) | ((*(uint32_t*)loc_8006C808) & 0xFFFF);

    // the LBA table of the XAS file is usually stored right below the BNS table, so we look for that one
    // in order to determine the size
    uintptr_t loc_8006B194 = pattern::get_first("40 18 02 00 ? ? 02 3C ? ? 42 24 21 18 62 00 21 98 83 00", 0);
    if (!loc_8006B194)
        return -3;

    // look for the XAS LBA table, in JP build it's at 0x80024FE4
    // lui $v0, 0x8002
    uintptr_t loc_8006B1A8 = loc_8006B194 + 0x14;
    // addiu $s6, $v0, 0x4FE4
    uintptr_t loc_8006B1B0 = loc_8006B194 + 0x1C;

    uintptr_t xasLbaTable = (((*(uint32_t*)loc_8006B1A8) & 0xFFFF) << 16) | ((*(uint32_t*)loc_8006B1B0) & 0xFFFF);

    // determine the count - each LBA entry is 8 bytes long (4 bytes LBA + 4 bytes size)
    uintptr_t lbaTableSize = xasLbaTable - lbaTable;

    *outCount = lbaTableSize / (sizeof(uint32_t) * 2);

    // the output position is the position in file, so we have to compensate for that
    *outPos = lbaTable - psxTextBase + psxExeTextStart;

    free(exeBuffer);
    return 0;
}

const char* DetectExtension(char* buf, size_t size)
{
    if (!size)
        return "bin";

    if (*(uint32_t*)(buf) == VAB_HEADER_MAGIC)
    {
        uint32_t vabsize = *(uint32_t*)(&buf[0xC]);
        if (vabsize <= size)
            return "vab";
        return "vh";
    }

    if (*(uint32_t*)(&buf[8]) == MODEL_HEADER_MAGIC)
    {
        return ".3dm";
    }

    uint32_t arcfilecount = *(uint32_t*)(buf);

    if ((arcfilecount <= 0xFF) && (arcfilecount))
    {
        return "arc";
    }

    return "bin";
}

int ExtractTekken3BinStream(std::filesystem::path BinsPath, std::filesystem::path ExePath, size_t exePosLBATable, size_t fileCount, std::filesystem::path outFolder)
{
    std::vector<uint32_t> lbas;
    std::vector<uint32_t> sizes;

    std::ifstream exefile;
    std::ifstream binsfile;

    if (fileCount > UINT16_MAX)
    {
        std::cerr << "ERROR: " << "Can't have more than " << UINT16_MAX << " files!\n";
        return -9;
    }

    exefile.open(ExePath, std::ios::binary);
    if (!exefile.is_open())
    {
        char errstr[512];
        std::cerr << "ERROR: " << "Can't open exe file " << ExePath.string() << " for reading! Reason: " << strerror_s(errstr, errno) << '\n';
        return -1;
    }

    binsfile.open(BinsPath, std::ios::binary);
    if (!binsfile.is_open())
    {
        char errstr[512];
        std::cerr << "ERROR: " << "Can't open binstream file " << BinsPath.string() << " for reading! Reason: " << strerror_s(errstr, errno) << '\n';
        return -2;
    }

    exefile.seekg(exePosLBATable);
    for (int i = 0; i < fileCount; i++)
    {
        uint32_t pos = 0;
        uint32_t size = 0;

        if (exefile.eof())
            break;

        exefile.read((char*)&pos, sizeof(uint32_t));
        exefile.read((char*)&size, sizeof(uint32_t));

        lbas.push_back(pos);
        sizes.push_back(size);
    }
    exefile.close();

    std::ofstream ofile;
    if (!std::filesystem::exists(outFolder))
        std::filesystem::create_directories(outFolder);

    for (int i = 0; i < lbas.size(); i++)
    {
        uint32_t lba = lbas.at(i);
        uint32_t size = sizes.at(i);
        size_t pos = lba * 0x800;


        std::filesystem::path outPath = outFolder;
        char filename[1024];
        sprintf_s(filename, "%d.bin", i);
        outPath.append(filename);

        std::cout << "LBA: " << "0x" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << lba
            << " | POS: " << "0x" << std::setw(8) << std::setfill('0') << std::uppercase << std::hex << pos
            << " | SIZE: " << "0x" << std::setw(8) << std::setfill('0') << std::uppercase << std::hex << size;
               

        char* filebuf = (char*)malloc(size);
        memset(filebuf, 0, size);
        binsfile.seekg(pos);
        binsfile.read(filebuf, size);

        outPath.replace_extension(DetectExtension(filebuf, size));

        std::cout << " ==> " << outPath.string() << '\n';

        ofile.open(outPath, std::ios::binary);
        if (!ofile.is_open())
        {
            std::cout << '\n';
            char errstr[512];
            std::cerr << "ERROR: " << "Can't open file " << outPath.string() << " for writing! Reason: " << strerror_s(errstr, errno) << '\n';
            continue;
        }
        ofile.write(filebuf, size);

        binsfile.clear();
        ofile.flush();
        ofile.close();
        free(filebuf);
    }


    binsfile.close();

    std::cout << "Extracted " << fileCount << " files.\n";

    return 0;
}

int PackTekken3BinStream(std::filesystem::path InFolder, std::filesystem::path ExePath, size_t exePosLBATable, size_t fileCount, std::filesystem::path outBins)
{
    std::vector<uint32_t> lbas;
    std::vector<uint32_t> sizes;
    std::vector<std::filesystem::path> files;

    if (fileCount > UINT16_MAX)
    {
        std::cerr << "ERROR: " << "Can't have more than " << UINT16_MAX << " files!\n";
        return -19;
    }

    std::ofstream binsfile;
    binsfile.open(outBins, std::ios::binary);
    if (!binsfile.is_open())
    {
        char errstr[512];
        std::cerr << "ERROR: " << "Can't open binstream file " << outBins.string() << " for writing! Reason: " << strerror_s(errstr, errno) << '\n';
        return -11;
    }

    size_t fcount = fileCount;

    for (const auto& entry : std::filesystem::directory_iterator(InFolder))
    {
        if (!isNumericFilename(entry.path()))
        {
            std::cout << "WARNING: ignoring file " << entry.path().string() << " because it is not numerically named...\n";
        }
        else
        {
            files.push_back(entry.path());
        }
    }

    if (files.size() <= 0)
    {
        std::cerr << "ERROR: " << "Failed to iterate directory (size: " << files.size() << "): " << InFolder.string() << '\n';
        return -12;
    }

    if (fileCount < files.size())
    {
        std::cout << "WARNING: " << "Too many files in directory!" << " Expected: " << fileCount << ", found: " << files.size() << '.' << " Using the first " << fcount << " (numerically sorted) files...\n";
    }
    else if (fileCount > files.size())
    {
        fcount = files.size();
        std::cout << "WARNING: " << "Too few files in directory!" << " Expected: " << fileCount << ", found: " << files.size() << '.' << " Using the first " << fcount << " (numerically sorted) files...\n";
    }

    std::sort(files.begin(), files.end(), numericFilenameComparator);

    for (int i = 0; i < fcount; i++)
    {
        std::cout << files[i].string();

        std::ifstream ifile;
        ifile.open(files[i], std::ios::binary);
        if (!ifile.is_open())
        {
            std::cout << '\n';
            char errstr[512];
            std::cerr << "ERROR: " << "Can't open file " << files[i].string() << " for reading! Reason: " << strerror_s(errstr, errno) << '\n';
            continue;
        }
        uintmax_t pos = binsfile.tellp();
        uintmax_t filesize = std::filesystem::file_size(files[i]);
        uintmax_t lba = 0;
        if (pos)
        {
            lba = pos / 0x800;
        }

        std::cout << " ==> " << "LBA: " << "0x" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << lba
            << " | POS: " << "0x" << std::setw(8) << std::setfill('0') << std::uppercase << std::hex << pos
            << " | SIZE: " << "0x" << std::setw(8) << std::setfill('0') << std::uppercase << std::hex << filesize << '\n';

        lbas.push_back(lba);
        sizes.push_back(filesize);

        if (filesize == 0)
        {
            ifile.close();
            continue;
        }

        char* filebuf = (char*)malloc(filesize);
        ifile.read(filebuf, filesize);
        ifile.close();

        binsfile.write(filebuf, filesize);
        binsfile.flush();
        
        uintmax_t postwrite_pos = binsfile.tellp();
        size_t newpos = alignTo0x800(postwrite_pos);
        binsfile.seekp(newpos, std::ios::beg);

        free(filebuf);
    }

    // write padding at the end
    binsfile.seekp(-1, std::ios::cur);
    binsfile.put(0);

    binsfile.close();
    std::cout << "Packed " << std::dec << fileCount << " files.\n";

    std::cout << "Updating executable: " << ExePath << '\n';
    if (!std::filesystem::exists(ExePath))
    {
        std::cerr << "ERROR: " << "File " << ExePath.string() << " does not exist!\n";
        return -13;
    }

    std::fstream exefile;
    exefile.open(ExePath, std::ios::binary | std::ios::in | std::ios::out);
    if (!exefile.is_open())
    {
        char errstr[512];
        std::cerr << "ERROR: " << "Can't open exe file " << ExePath.string() << " for reading & writing! Reason: " << strerror_s(errstr, errno) << '\n';
        return -14;
    }

    exefile.seekp(exePosLBATable);
    for (int i = 0; i < lbas.size(); i++)
    {
        uint32_t lba = lbas[i];
        uint32_t size = sizes[i];

        exefile.write((char*)&lba, sizeof(uint32_t));
        exefile.write((char*)&size, sizeof(uint32_t));
        exefile.flush();
    }

    exefile.close();

    std::cout << "Updated " << std::dec << lbas.size() << " entries.\n";


    return 0;
}

int main(int argc, char** argv)
{
    std::cout << "Tekken 3 BinStream Extractor & Packer\n\n";

    if (argc < (3 + 1))
    {
        std::cout << "USAGE: " << argv[0] << " BinStreamPath ExePath OutFolder" << '\n';
        std::cout << "USAGE (manual pos): " << argv[0] << " -m BinStreamPath ExePath LBATablePosInExe FileCount OutFolder" << '\n';
        std::cout << "USAGE (pack): " << argv[0] << " -p InFolder ExePath OutBinStream" << '\n';
        std::cout << "USAGE (manual pos pack): " << argv[0] << " -mp InFolder ExePath LBATablePosInExe FileCount OutBinStream" << '\n';
        return -3;
    }

    char* exename = argv[2];
    bool bPacking = false;

    if ((argv[1][0] == '-') && (argv[1][1] == 'p') && (argv[1][2] == '\0'))
    {
        exename = argv[3];
        bPacking = true;
    }


    if ((argv[1][0] == '-') && (argv[1][1] == 'm') && (argv[1][2] == '\0'))
        return ExtractTekken3BinStream(argv[2], argv[3], std::stoul(argv[4], 0, 0), std::stoul(argv[5], 0, 0), argv[6]);

    if ((argv[1][0] == '-') && (argv[1][1] == 'm') && (argv[1][2] == 'p'))
        return PackTekken3BinStream(argv[2], argv[3], std::stoul(argv[4], 0, 0), std::stoul(argv[5], 0, 0), argv[6]);


    uintptr_t lbapos = 0;
    uint32_t lbacount = 0;

    int findret = FindLBATable(exename, &lbapos, &lbacount);
    if (findret < 0)
    {
        if (findret == -3)
        {
            std::cerr << "ERROR: " << "Can't find the XAS LBA table in the executable!\n";
            return -4;
        }

        if (findret == -2)
        {
            std::cerr << "ERROR: " << "Can't find the LBA table in the executable!\n";
            return -5;
        }

        if (findret == -1)
        {
            std::cerr << "ERROR: " << "Can't open the executable!\n";
            return -6;
        }
    }

    std::cout << "LBA table found at: 0x" << std::uppercase << std::hex << lbapos << ", count: " << std::dec << lbacount << '\n';

    if (bPacking)
        return PackTekken3BinStream(argv[2], exename, lbapos, lbacount, argv[4]);

    return ExtractTekken3BinStream(argv[1], exename, lbapos, lbacount, argv[3]);
}
