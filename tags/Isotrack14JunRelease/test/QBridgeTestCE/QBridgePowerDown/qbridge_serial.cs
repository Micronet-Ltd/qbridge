using System;
using CITT_CRC;
using j1708TestCE;

namespace qbridge_serial_com //QBridgeTestCE
{
    	public enum ack_enums{
			ack_ok = '0',
			ack_duplicate_packet  = '1',
			ack_invalid_packet    = '2',
			ack_invalid_command   = '3',
			ack_invalid_data      = '4',
			ack_unable_to_process = '5',
			ack_can_bus_fault     = '6',
			ack_overflow_occurred = '7',
		};

		public enum debuginfotype{
			general = 0,
			j1708_log = 1,
			mid_info = 2
		};

		public enum getinfotype{
			buildinfo = 0,
			j1708info = 1,
			caninfo = 2,
			rst_ua = 3,	//reset the unit attention stuff pertaining to queue overflow
		};

    public class qbridge_serial
    {
        public qbridge_serial()
        {
	        qbcrc = new crc();
	        id = 0;
	        status = 0;
	        err = null;
	        rdata = new byte[256];
            trash = new byte[2];
            waiting_for_ack = false;
        }

        private crc qbcrc;
        public byte[] rdata;
        public byte id;
        public byte status;
        public string err;
        public string ret_info;
        public System.IO.Ports.SerialPort mySerial;
        public bool waiting_for_ack;
        public Form1 myf;

        public byte[] trash;



        public void init( ) { docmd((byte)'@',0, ref trash); }
        public void ack(byte myid, ack_enums ackcode, ref byte[] data )
        {
            byte[] d = new byte[2];
            d[0] = (byte)(myid & 0xff);
            d[1] = (byte)ackcode;
            docmd((byte)'A', 2, ref d ); 
        }
        public void mid_filter_enable(bool enable) { byte[] d = new byte[1]; d[0] = (byte)(enable ? 0xff : 0); docmd((byte)'B', 1, ref d); }
        //public void set_mid( bool enable, byte *midlist ) { docmd('C'); }
        public void send1708_packet( byte packet_len, ref byte[] packet_info )
        { 
	        docmd((byte)'D', packet_len , ref packet_info);
	        if( status == 0 ){
		        if( rdata[0] != 5 ){
			        rdata[0] = 0;
			        status = 1;
			        err = "bad number of bytes returned as ack to J1708 send command!";
		        }else{
			        byte q = rdata[1];
			        int id = rdata[2] | (rdata[3]<<8) | (rdata[4]<<16) | (rdata[5]<<24);
                    ret_info = String.Format("J1708 que positions = {0},  ID assigned to that J1708 packet = {1:x}", q, id);
			        rdata[0] = 0xff;
		        }
	        }
        }
        //public void receive1708_packet( byte **packet_info ) { docmd((byte)'E'); }
        //public void enable_1708_xmit_confirm( bool enable ) { docmd((byte)'F'); }
        ////public void j1708_xmit_confirm(){docmd((byte)'G');}
#if do_fw_upgrade
        public char[] txt; //[100];
        public void upgrade_firmware( System::Windows::Forms::ProgressBar ^progress )
        {
	    docmd('H', 0, nullptr);
	    if( status != 0 ) {
		    //it may be that the comport isn't open
		    if( mySerial->IsOpen == false ){
			    status = 0x10;
			    err = "com port not open";
			    return;
		    }
		    //it may be that the bootloader is running, try to talk to him that way
		    String ^tmp = mySerial->NewLine;
		    mySerial->NewLine="\n";
		    mySerial->WriteLine("");
		    System::Threading::Thread::Sleep(700);
		    mySerial->DiscardInBuffer();
		    mySerial->WriteLine("abc"); //we expect a NAK after this
		    if( getbl_ack() != 0x15 ){mySerial->NewLine=tmp; return;}
		    status=0;
		    err=nullptr;
		    mySerial->NewLine=tmp;
	    }
	    //at this point, we believe we are in bootloader mode, ready to recieve new firmware
	    //present the dialog box to query what to download
	    System::Windows::Forms::FileDialog ^fd = gcnew System::Windows::Forms::OpenFileDialog();
	    fd->Filter = "SRec Files|*.srec";
	    if( fd->ShowDialog() == System::Windows::Forms::DialogResult::OK ) {
		    //now send the stuff one line at a time
		    System::IO::FileStream ^f = gcnew System::IO::FileStream(fd->FileName, System::IO::FileMode::Open);
		    System::IO::StreamReader ^sr = gcnew System::IO::StreamReader(f);
		    if( progress != nullptr ){
			    progress->Maximum = (int)f->Length;
			    progress->Minimum = 0;
			    progress->Value = 0;
			    progress->Visible = true;
			    progress->Show();
		    }
		    String ^tmp = mySerial->NewLine;
		    mySerial->NewLine = "\n";
		    mySerial->DiscardInBuffer();
		    for(int lc=0;;lc++){
			    String ^l = sr->ReadLine();
			    if( progress != nullptr) progress->Value = (int)f->Position;
			    if( l == nullptr ) break;
			    mySerial->WriteLine(l);
			    if( int nnnn = getbl_ack() ) {
				    status = 0x100;
				    //char txt[100];
				    sprintf_s(txt,100,"bad ack during programming (%d) (%d)",lc, nnnn);
				    err = txt;//"bad ack during programming";
				    break;
			    }
		    }
		    sr->Close();
		    f->Close();
		    mySerial->NewLine=tmp;
	    }

    }
#endif //upgrade_fw
        public void reset_qbridge( ) { docmd((byte)'I',0, ref trash); }
        public void enable_xmit_confirm(bool enable) { byte[] d = new byte[1]; d[0] = (byte)(enable ? 0xff : 0); docmd((byte)'F', 1, ref d); }
        public void power_down(UInt32 param) { byte[] d = new byte[1]; d[0] = (byte)(param); docmd((byte)'O', 1, ref d); }

        //###########################################
        //### Send a CAN packet
        //###########################################
        public void sendCANpacket( UInt32 id, int CANdlen, ref byte[] data )
        { 
	        byte[] myCANdata = new byte[100];
	        int cnt = 0;
	        if((id & 0x80000000) != 0) { //EXTENDED or STANDARD identifier (msbit set = standard, not set = extended)
		        myCANdata[cnt++] = 0;
		        myCANdata[cnt++] = (byte)(id & 0xff);
		        myCANdata[cnt++] = (byte)((id>>8) & 0xff);
	        }else{
		        myCANdata[cnt++] = 1;
		        myCANdata[cnt++] = (byte)(id & 0xff);
		        myCANdata[cnt++] = (byte)((id>>8) & 0xff);
		        myCANdata[cnt++] = (byte)((id>>16) & 0xff);
		        myCANdata[cnt++] = (byte)((id>>24) & 0xff);
	        }
	        for( int i= 0; i<CANdlen; i++ )
		        myCANdata[cnt++] = data[i];//here is the data
	        docmd((byte)'J',(byte)cnt,ref myCANdata);
	        if( status == 0 ){
		        if( rdata[0] != 5 ){
			        rdata[0] = 0;
			        status = 1;
			        err = "bad number of bytes returned as ack to CAN send command!";
		        }else{
			        byte q = rdata[1];
			        int id2 = rdata[2] | (rdata[3]<<8) | (rdata[4]<<16) | (rdata[5]<<24);
                    ret_info = String.Format("CAN que positions = {0},  ID assigned to that CAN packet = {1:x}", q, id2);
			        rdata[0] = 0xff;
		        }
	        }
        }

        public void sendCANcontrol( int CANdlen, ref byte[] data ){ docmd((byte)'L', (byte)CANdlen, ref data); }

        public void get_info(ref byte[] data){ docmd((byte)'M', 1, ref data); }


        public void info_request_to_debug_port(debuginfotype x) { byte[] data = new byte[1]; data[0] = (byte)x; docmd((byte)'*', 1, ref data); }

        //###########################################
        //### Do the PJ debug thing
        //###########################################
        //public void pj( byte *x ) { docmd((byte)'p', x[0] & 0xff, &x[1] );}

        public bool test_if_in_bootloader_mode( )
        {
		    //it may be that the comport isn't open
		    if( mySerial.IsOpen == false ){
			    status = 0x10;
			    err = "com port not open";
			    return false;
		    }
		    //it may be that the bootloader is running, try to talk to him that way
		    status = 0x01;
		    err = "wrong bootloader response";
		    String tmp = mySerial.NewLine;
		    mySerial.NewLine="\n";
		    mySerial.WriteLine("");
		    mySerial.DiscardInBuffer();
		    mySerial.WriteLine("abc"); //we expect a NAK after this
		    if( getbl_ack() != 0x15 ){mySerial.NewLine=tmp; return false;}
		    status=0;
		    err=null;
		    mySerial.NewLine=tmp;
		    return true;
        }

        //########################################################################################################
        //########################################################################################################
        //#### Here is the work of setting up the command and transfering it to the qbridge
        //########################################################################################################
        //########################################################################################################
        public void docmd( byte cmd, byte len, ref byte[] data )
        {
	        byte[] x = new byte[100];
	        int n;
            int cnt=0;

	        if( len > 90 ){ status = 0x80; err="len of data too large for cmd"; return;}
	        x[0] = 0x02;
	        x[2] = cmd;
	        if( cmd == (byte)'A' ){ //ack behaves differently
		        x[3] = data[cnt++];	//ack id was placed as part of our data... update count and point to real data
		        len -= 1;
	        }else{
		        x[3] = id++;
		        id &= 0xff;
	        }
	        x[1] = (byte)(len+6);
	        for( n=0; n<len; n++ )
		        x[4+n] = data[cnt+n];
            UInt32 mycrc = qbcrc.compute_crc( ref x, 0, len+4 );
	        x[4+n] = (byte)(mycrc & 0xff);
	        x[4+n+1] = (byte)((mycrc >> 8) & 0xff);


	        byte[] y = new byte[100];
	        for( n=0; n<len+6; n++ )
		        y[n] = x[n];
	        if( mySerial.IsOpen == false ){
		        status = 0x10;
		        err = "com port not open";
		        return;
	        }
	        mySerial.DiscardInBuffer();
	        status = 1;
            waiting_for_ack = true;
            mySerial.Write(y, 0, len + 6);
	        if( cmd == 'A' ){
		        status = 0;
		        rdata[0] = 0;
		        err="waiting for ack";
                waiting_for_ack = false;
		        return;	//no need to wait for ACK if we are doing the ack
	        }
	        getack();
        }

        public void getack( )
        {
	        byte[] y = new byte[250];
	        byte myb, ackcode;
	        int rstate = 0;
	        int n, rl;

	        //let's wait a while to see if he acks us
            waiting_for_ack = true;
            n = 0;
	        rstate = 0;
	        rdata[0] = 0;
	        err="waiting for ack";
            myb = 0; //fix compile warning about unassigned local
            rl = 0;  //fix compile warning about unassigned local
            ackcode = 0; //fix compile warning about unassigned local
	        while( true ) {
		        if( rstate < 8 ) {
			        Int64 d1 = DateTime.Now.Ticks;
                    while (!myf.isSerialDataAvail()) {//mySerial.BytesToRead == 0 ) {
				        Int64 d2 = DateTime.Now.Ticks;
                        System.Threading.Thread.Sleep(2);
				        if( (d2 - d1) > 7000000 ) {
					        status |= 0x04;
					        err="Timeout waiting for ack packet";
					        rstate = 9;
                            waiting_for_ack = false;
                            return;
				        }
			        }
                    y[n++] = myb = myf.readSerialData();//(byte)mySerial.ReadByte();
		        }
		        switch( rstate ) {
			        case 0:
				        if( myb == 0x02 ) rstate = 1; else {rstate = 9; err="badly formed ack? No STX char";}
				        break;
			        case 1:
				        rl = myb;
				        if( rl >= 7 ) rstate = 2; else {rstate = 9; err="badly formed ack? Len < 7";}
				        break;
			        case 2:
				        if( myb == 'A' ) rstate = 3; else {rstate = 9; err="receiving something other than ack";}
				        break;
			        case 3:
				        if( myb == ((id-1) & 0xff)) rstate = 4; else {rstate = 9;
				        byte q = rdata[1];
				        //int id = rdata[2] | (rdata[3]<<8) | (rdata[4]<<16) | (rdata[5]<<24);
				        ret_info = string.Format("Wrong ID being acked (got {0}, expected {1})", myb, id-1);
                        rdata[0] = 0xff;
				        //err="wrong id being acked";}
				        }
				        break;
			        case 4:
				        ackcode = myb;
				        rstate = 5;
				        rl -= 4;//because it gets dec'd right away   5;
				        //fall thru to check length
                        if (--rl <= 2) rstate = 6;
                        break;
                    case 5:
				        if( --rl <= 2 ) rstate = 6;
				        break;
			        case 6: //receiving CRC
				        rstate = 7;
				        break;
			        case 7: //last of CRC
				        {
                        UInt32 rcrc = qbcrc.compute_crc(ref y,0,n-2);
				        if( rcrc != ((y[n-1]<< 8) | y[n-2])){ err="bad ack crc"; rstate=9;} else rstate = 8;
				        break;
				        }
			        case 8: //all is well
				        status = 2;
				        switch( ackcode ) {
					        case (byte)ack_enums.ack_ok:
						        status = 0;
						        err=null;
						        break;
                            case (byte)ack_enums.ack_duplicate_packet:
						        err="duplicate packet ack";
						        break;
                            case (byte)ack_enums.ack_invalid_packet:
						        err="invalid packet ack";
						        break;
                            case (byte)ack_enums.ack_invalid_command:
						        err="invalid command ack";
						        break;
                            case (byte)ack_enums.ack_invalid_data:
						        err="invalid data ack";
						        break;
                            case (byte)ack_enums.ack_unable_to_process:
						        err="unable to process ack";
						        break;
					        default:
						        err ="unknown ack received";
						        break;
				        }
				        rdata[0] = (byte)(n-7);
				        for( int i=0; i<n-7; i++ )
					        rdata[i+1] = y[i+5];
                        waiting_for_ack = false;
                        return;
			        case 9: //error state
				        status = 4;
                        waiting_for_ack = false;
                        return;
		        }
	        }
            waiting_for_ack = false;
        }
        int getbl_ack( )
        {
            while( true ) 
            {
	            Int64 d1 = DateTime.Now.Ticks;
	            while( mySerial.BytesToRead == 0 ) {
		            Int64 d2 = DateTime.Now.Ticks;
		            if( (d2 - d1) > 100000000 ) {
			            status |= 0x04;
			            err="Timeout waiting for bootloader ack";
			            return( 1000 );
		            }
	            }
	            byte myb = (byte)mySerial.ReadByte();		
	            if( myb == 6 ) {status = 0; err=null; return( 0 ); }
	            err="bad ack value from bootloader";
	            status |= 4;
	            return  myb;
            }
        }

}//end of class
}//end of namespace
