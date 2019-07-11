#include "packet.hpp"
#include <cstring>

using namespace std;

const char PacketHeader::header_magic[PacketHeader::MagicLength::value] = "BMTXHDR";

void set_magic(PacketHeader &header) {
    memcpy(header.magic, PacketHeader::header_magic, PacketHeader::MagicLength::value);
}