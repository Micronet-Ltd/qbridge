using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;

namespace CITT_CRC //QBridgeTestCE
{
    //this implements the CITT CRC
    class crc
    {
        public crc()
        {
            crc_table = new UInt16[256];
            //createCRCTable((table), 16, 0x1021, FALSE)
            for (UInt32 i = 0; i < 256; i++)
            {
                UInt32 reg = (UInt32)(i << 8);
                for (int j = 0; j < 8; j++)
                    reg = (UInt32)((reg << 1) ^ (((reg & 0x8000) != 0)? 0x1021:0));
                crc_table[i] = (UInt16)(reg & 0xffff);
            }
        }
        public UInt32 compute_crc( ref byte[] x )
        {
            return compute_crc( ref x, 0, x.Length);
        }
        public UInt32 compute_crc( ref byte[] x, int start, int len )
        {
            UInt32 crc=0xffff;
            for( int i=0; i<len; i++ )
                crc = crc_table[((crc >> 8) ^ x[i+start]) & 0xff] ^ (crc << 8);
            return crc & 0xffff;
        }

        private UInt16[] crc_table;
    }
}
