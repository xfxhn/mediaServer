

#include "FLVHeader.h"
#include "bitStream/writeStream.h"

int FLVHeader::write(WriteStream &ws) {

    ws.setString(Signature, 3);

    ws.writeMultiBit(8, Version);
    //Reserved
    ws.writeMultiBit(5, 0);
    ws.writeMultiBit(1, TypeFlagsAudio);
    //Reserved
    ws.writeMultiBit(1, 0);
    ws.writeMultiBit(1, TypeFlagsVideo);
    ws.writeMultiBit(32, DataOffset);
    return 0;
}
