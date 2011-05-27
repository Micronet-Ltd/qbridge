// j1708_test.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"
#include "string.h"
#include "stdio.h"

using namespace j1708_test;

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	// Create the main window and run it
	Application::Run(gcnew Form1());
	return 0;
}


void Form1::newmaindata_handler(void){
	if( mainSerial->IsOpen == false ) return;
	if( mainSerial->BytesToRead != 0 ) {
		int x = mainSerial->BytesToRead;
		array<unsigned char, 1> ^zz = gcnew cli::array<unsigned char,1>(x);
		mainSerial->Read(zz,0,x);
		for( int n=0; n<x; n++ ){
			mainq[mainqe++] = zz[n];
			if( mainqe >= 10000 ) mainqe = 0;
		}
	}

	int myql = (mainqe >= mainqs) ? mainqe-mainqs : mainqe+10000 - mainqs;
	//if( myql > 0 ) mytick++; else mytick = 0;
	//if( mytick > 10 ){	//forget this message
	//	int i;
	//	for( ; mainqs != mainqe; ){
	//		mainqs = mainqs+1;//skip current start of message
	//		if( mainqs >= 10000 ) mainqs = 0;
	//		if( mainq[mainqs] == 0x02 )
	//			break;
	//	}
	//	myql = mainqe >= mainqs ? mainqe-mainqs : mainqe+10000 - mainqs;
	//	mytick = 0;
	//}
	if( myql > 2 ){	//enough data that we can look for start of message and length?
		if( mainq[mainqs] == 0x02 ){
			int l = mainq[(mainqs+1) % 10000 ];
			if( myql >= l ) {//we have a complete message
				dequemain();
				handle_packet();
			}
		}else{ //skip this stuff until you find a start of message (or end of buffer)
			//this->richTextBox1->AppendText("skipping packet due to no STX\n");
			for( ; mainqs != mainqe; ){
				if( mainq[mainqs] == 0x02 )
					break;
				else{// assume it is ascii and we can display this data
					wchar_t n[1];
					n[0] = mainq[mainqs];
					String ^x = gcnew String(n,0,1);
					this->richTextBox1->SelectionColor = Color::Blue;
					this->richTextBox1->AppendText(x);
					this->richTextBox1->SelectionColor = Color::Black;
				}
				mainqs = mainqs+1;//skip current start of message
				if( mainqs >= 10000 ) mainqs = 0;
			}
		}
	}
}

void Form1::newdebugdata_handler(void){
	wchar_t n[1];
	bool changed = false;
	while( debugqs != debugqe ) {
		this->richTextBox1->SelectionColor = Color::Green;
		changed = true;
		n[0] = debugq[debugqs++];
		String ^x = gcnew String(n, 0, 1);
		this->richTextBox1->AppendText(x);
		if( debugqs >= 10000 ) debugqs = 0;
	}
	if( changed ) this->richTextBox1->SelectionColor = Color::Black;
}
System::Void Form1::dequemain( void ){
	int l = mainq[(mainqs+1) % 10000 ];
//	if( l >= 400 ) ??? don't overrun buffer size
	mainqs = (mainqs + 2) % 10000;
	for( int i=0; i<l-2; i++ ){
		pkt[i] = mainq[mainqs];
		mainqs++;
		if( mainqs >= 10000 ) mainqs=0;
	}
	pktl = l-2;
}

System::Void Form1::handle_packet( void ){
	if( pktl < 6 ) return; //this is an invalid packet so ignore it
	//really should check CRC, but we're not going to...
	Byte id = pkt[1];
	switch( pkt[0] ){
		case 'K': //received CAN message
			do_can_receive( id, &pkt[2], pktl-4 );
			break;
		case 'G': //transmit confirm
			do_transmit_confirm( id, &pkt[2], pktl-4);
			break;
		case 'E': //received J1708 message
			do_j1708_receive( id, &pkt[2], pktl-4 );
			break;
		default:
			this->qbridge->ack(id,ack_invalid_command,nullptr);
			this->richTextBox1->AppendText("bad packet received\n");
			break;
	}
}

System::Void Form1::do_can_receive( Byte id, unsigned char *data, int datalen ){
	int l = datalen;
	if ( (l < 3) //this option requires at least a type identifier (1) and a CAN identifier (2 for the 11 bit version)
	  || (l > 13)//this option can have no more than a type id(1), CAN id (4), and 8 bytes of data
	  || (data[0] > 1) //identifier type can only be 0 or 1
	  || ((data[0] == 1) && (l < 5)) //extended CAN requires 4 byte identifier
	  || ((data[0] == 0) && (l > 11))//standard CAN, id=2, data max=8
	  ){
		this->qbridge->ack (id, ack_invalid_data, nullptr);
		this->richTextBox1->AppendText("bad CAN received data format\n");
		return;
	}

	int identifier;
	unsigned char *dptr;
    if( data[0] == 0 ){ //standard CAN identifier
        identifier = data[1] | (data[2] << 8);
        dptr = &data[3];
        l = l - 3;
    }else{ //extended CAN identifier
        identifier = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
        dptr = &data[5];
        l = l - 5;
    }
	this->qbridge->ack(id, ack_ok, nullptr);
	this->richTextBox1->AppendText(String::Format("Received CAN data (id={0,8:x} data len = {1} ", identifier, l));
	if( l > 0 ){
		this->richTextBox1->AppendText("Data: ");
		while( l-- ){
			this->richTextBox1->AppendText(String::Format("{0,2:x}", *dptr++));
		}
	}
	this->richTextBox1->AppendText(")\n");
}

System::Void Form1::do_transmit_confirm( Byte id, unsigned char *data, int datalen ){
	int l = datalen;
	if(l != 5){ //this option has success/failure flag, and id being confirmed
		this->qbridge->ack (id, ack_invalid_data, nullptr);
		this->richTextBox1->AppendText("bad transmit confirm data format\n");
		return;
	}
	//may someday want to keep track of outstanding ids and if this isn't one... generate an error
	this->qbridge->ack(id, ack_ok, nullptr);
	int identifier = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
	this->richTextBox1->AppendText(String::Format("Received transmit confirmation for id={0,8:x}\n", identifier));
}

System::Void Form1::do_j1708_receive( Byte id, unsigned char *data, int datalen ){
	this->qbridge->ack(id, ack_ok, nullptr);
	this->richTextBox1->AppendText("Received J1708 data: ");
	if( datalen > 0 ){
		while( datalen-- ){
			this->richTextBox1->AppendText(String::Format(" {0,2:x}", *data++));
		}
	}
	this->richTextBox1->AppendText("\n");
}


//int mytick=0;
System::Void Form1::timer1_Tick(System::Object^  sender, System::EventArgs^  e) {
		newdebugdata_handler();
		newmaindata_handler();
 }

System::Void Form1::mainSerial_DataReceived(System::Object^  sender, System::IO::Ports::SerialDataReceivedEventArgs^  e) {
			 	int x = mainSerial->BytesToRead;
				array<unsigned char, 1> ^zz = gcnew cli::array<unsigned char,1>(x);
				mainSerial->Read(zz,0,x);
				for( int n=0; n<x; n++ ){
					mainq[mainqe++] = zz[n];
					if( mainqe >= 10000 ) mainqe = 0;
				}
		 }
System::Void Form1::debugSerial_DataReceived(System::Object^  sender, System::IO::Ports::SerialDataReceivedEventArgs^  e) {
			 	int x = debugSerial->BytesToRead;
				array<unsigned char, 1> ^zz = gcnew cli::array<unsigned char,1>(x);
				debugSerial->Read(zz,0,x);
				for( int n=0; n<x; n++ ){
					debugq[debugqe++] = zz[n];
					if( debugqe >= 10000 ) debugqe = 0;
				}
		 }
System::Void Form1::richTextBox1_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
			 if( e->KeyCode == Keys::Enter ) {
				 e->Handled = true;
				 if( this->richTextBox1->SelectionLength != 0 ){
					 if(this->richTextBox1->GetLineFromCharIndex(this->richTextBox1->SelectionStart) == this->richTextBox1->Lines->Length-1)
						 this->richTextBox1->SelectionLength = 0;
				 }
				 if( this->richTextBox1->SelectionLength == 0 ) {
					 int myp = this->richTextBox1->SelectionStart;
					 int myp2 = this->richTextBox1->GetLineFromCharIndex(myp);
					 if( myp2 >= this->richTextBox1->Lines->Length ){this->richTextBox1->AppendText("\n"); return;}
					 String ^cmd = this->richTextBox1->Lines[myp2];
					 this->richTextBox1->AppendText("\n");
					 //here we do the command
					 dcmd( cmd );
					 if( this->qbridge->status != 0 ){
						 this->richTextBox1->SelectionColor = Color::Red;
						 if( this->qbridge->err == nullptr ){
							 this->richTextBox1->AppendText("Error unknown");
						 }else{
							 String ^mys = gcnew String(this->qbridge->err);
							 this->richTextBox1->AppendText(mys);
						 }
						 this->richTextBox1->SelectionColor = Color::Black;
						 this->richTextBox1->AppendText("\n");
					 }
					 //display the returned data from the command.... problem pj... the returned data may or may not be text
					 if( this->qbridge->rdata[0] != 0 ){
						 this->richTextBox1->SelectionColor = Color::Blue;
						 String ^t = gcnew String((const char *)this->qbridge->rdata, 1, this->qbridge->rdata[0] & 0xff);
						 this->richTextBox1->AppendText( t );
						 this->richTextBox1->SelectionColor = Color::Black;
						 this->richTextBox1->AppendText("\n");
					 }
					 //see if there is debug data to display
					 for( int mytime = 0; mytime < 30; mytime++ ){
						System::Threading::Thread::Sleep(5);
						newmaindata_handler();
					 }
					 newdebugdata_handler();
					 //command is done
					 this->richTextBox1->AppendText(cmd);
					 this->richTextBox1->SelectionStart = this->richTextBox1->TextLength - cmd->Length;
					 this->richTextBox1->SelectionLength = cmd->Length;
				 }
			 }
		 }

System::Void Form1::dofw(void){
			this->toolStripProgressBar1->Value = 0;
			this->toolStripProgressBar1->Visible = true;
			this->toolStripStatusLabel1->Visible = true;
			this->qbridge->upgrade_firmware( this->toolStripProgressBar1->ProgressBar );
			this->toolStripProgressBar1->Visible = false;
			this->toolStripStatusLabel1->Visible = false;
		}

System::Void Form1::dumpJ1708RxQueue(void){
	UInt32 j1708RxQueueStart = 0x20002600;
	UInt32 hd, tl;
	UInt32 dptr;
	try{
		hd = readword( j1708RxQueueStart );
		tl = readword( j1708RxQueueStart + 4);
		this->richTextBox1->AppendText(String::Format("Dumping J1708 Rx Queue: start index {0}, end index {1}\n", hd, tl));
		dptr = j1708RxQueueStart+8;
		for( int cnt=0; cnt<128; cnt++ ){
			UInt32 msgdptr = dptr + (hd * (21+8+3));
			UInt32 tmp = readword( msgdptr );
			int priority = tmp & 0xff;
			int len = (tmp >> 8) & 0xff;
			int txcnfrm = (tmp>>16) & 0xff;
			int id = readword( msgdptr + 4 );
			this->richTextBox1->AppendText(String::Format("#{0,3}: {1,3} {2,2} {3} {4,5} data:", hd, priority, len, txcnfrm, id));
			for( int i=0; i<len/4; i++ ){
				tmp = readword( msgdptr + 8 + i*4 );
				this->richTextBox1->AppendText(String::Format("{0,2:x} {1,2:x} {2,2:x} {3,2:x} ", (tmp & 0xff), ((tmp>>8) & 0xff), ((tmp>>16) & 0xff), ((tmp>>24) & 0xff)));
			}
			if( len % 4 ){
				tmp = readword( msgdptr + 8 + (len/4)*4 );
				for( int i=0; i<(len %4); i++ ){
					this->richTextBox1->AppendText(String::Format("{0,2:x} ", (tmp & 0xff)));
					tmp = (tmp >> 8) & 0xff;
				}
			}
			this->richTextBox1->AppendText("\n");
			//dptr += 21+8+3;
			hd++;
			if( hd >= 128 ) hd = 0;
		}
	}
	catch( System::ArgumentException ^px){
		//System::Windows::Forms::MessageBox::Show(String::Format("Sorry, {0}", px->Message),
		//			 "Bad response from qbridge...", System::Windows::Forms::MessageBoxButtons::OK, System::Windows::Forms::MessageBoxIcon::Exclamation);
	}
}

System::Void Form1::dcmd( String ^cmdline ) {
	String ^cmd;
	this->qbridge->status = 0;
	this->qbridge->err = "";
	this->qbridge->rdata[0] = 0;

	cmdline = cmdline->Trim();
	String ^tmpstr = " \t,(){}[]";
	array<wchar_t,1> ^delims = tmpstr->ToCharArray();
	array<String ^,1> ^args = cmdline->Split(delims);
	//int cl = cmdline->IndexOfAny(delims);
	//if( cl < 0 ) cl=cmdline->Length;
	//cmd = cmdline->Substring(0,cl);
	//int cr = cmdline->Length - (cl+1);
	//if( cr > 0 ) cmdline = cmdline->Substring(cl+1, cr); else cmdline="";
	//cmdline = cmdline->Trim();
	cmd = args[0];

	cmd = cmd->ToLower();
	if( cmd->Length == 0 ) return;

	if( cmd == "mid" ){
		if( args->Length == 2 ) {
			if( args[1]->ToLower() == "on" )
				this->qbridge->mid_filter_enable(true);
			else if (args[1]->ToLower() == "off" )
				this->qbridge->mid_filter_enable(false);
			else
				this->richTextBox1->AppendText("huh?");
		}else{
			this->qbridge->info_request_to_debug_port(mid_info);
		}
	}
	 else if( cmd == "inq" )
		 this->qbridge->info_request_to_debug_port(general);
	 else if( cmd == "log")
		 this->qbridge->info_request_to_debug_port(j1708_log);
	 else if( cmd == "j1708rxqueue")
		 this->dumpJ1708RxQueue();
	 else if( cmd == "fw")
		 dofw();//this->qbridge->upgrade_firmware(nullptr);
	 else if( cmd == "ini" || cmd=="init")
		 this->qbridge->init();
	 else if( (cmd == "qui") || (cmd == "exit") )
		 Application::Exit();
	 else if( cmd == "cls" )
		 this->richTextBox1->Text="";
	 else if( cmd =="ctx" )
		 this->qbridge->sendCANpacket(0x1555555, 0, nullptr);
	 else if( cmd == "ctm" )
		 this->qbridge->sendCANpacket( Int32::Parse(args[1],System::Globalization::NumberStyles::HexNumber), 0, nullptr );
	 else if( cmd == "jtx" ){
		 Byte d[] = {8,111,'h','e','l','p',' ','m','e'};
		 if( args->Length >= 2 ){
			int x = Int32::Parse(args[1],System::Globalization::NumberStyles::HexNumber);
			d[0] = x & 0x0f;}
		 this->qbridge->send1708_packet(7, (unsigned char *)d);//"\8\111help me");
	 }
	 else if( cmd == "loopback" )
		 this->qbridge->sendCANcontrol( 2, (unsigned char *)"dL");
	 else if( cmd == "silent" )
		 this->qbridge->sendCANcontrol( 2, (unsigned char *)"dS");
	 else if( cmd == "hotselftest" )
		 this->qbridge->sendCANcontrol( 2, (unsigned char *)"dH");
	 else if( cmd == "nodebug" || cmd == "debugoff" || cmd == "debugnorm" )
		 this->qbridge->sendCANcontrol(2, (unsigned char *)"dN");
	 else if( cmd == "baud1m" )
		 this->qbridge->sendCANcontrol(2,(unsigned char *)"b1");
	 else if( cmd == "baud512k")
		 this->qbridge->sendCANcontrol(2,(unsigned char *)"b2");
	 else if( cmd == "baud250k")
		 this->qbridge->sendCANcontrol(2,(unsigned char *)"b3");
	 else if( cmd == "baud125k")
		 this->qbridge->sendCANcontrol(2,(unsigned char *)"b4");
	 else if( cmd == "baudnorm")
		 this->qbridge->sendCANcontrol(2,(unsigned char *)"b0");
	 else if( cmd == "ena_txc" )
		 this->qbridge->enable_xmit_confirm( true );
	 else if( cmd == "dis_txc" )
		 this->qbridge->enable_xmit_confirm( false );
	 else if( cmd == "ji" )
		 j1708_info();
	 else if( cmd == "ci" )
		 can_info();
	 else if( cmd == "rua")
		 rst_info();
	 else if( cmd == "info" )
		 inq_info();
	 else if( cmd == "dis_filt" || cmd == "dis_cfilt")
		 this->qbridge->sendCANcontrol(2,(unsigned char *)"e\0\0");
	 else if( cmd == "ena_filt" || cmd == "ena_cfilt")
		 this->qbridge->sendCANcontrol(2,(unsigned char *)"e\1\0");
	 else if( cmd == "dis_jfilt" )
		 this->qbridge->mid_filter_enable(false);
	 else if( cmd == "ena_jfilt" )
		 this->qbridge->mid_filter_enable(true);
	 else if( cmd == "gf"){ //get filter
		 this->qbridge->sendCANcontrol(1,(unsigned char *)"g");
		 //display_filter_data();
		 if( this->qbridge->status == 0 ){
			 int num = this->qbridge->rdata[0];
			 this->qbridge->rdata[0] = 0;	//so no one attempts to display this data
			 //this->richTextBox1->AppendText("\n");
			 Byte *d = this->qbridge->rdata;
			 d++;
			 while( num ){
				 if( d[0]==0 ) {	//standard
					 if( num < 5 ){
						 this->richTextBox1->AppendText("QBridge error in returned standard filter data size");
						 return;
					 }
					 UInt32 id = d[1] | d[2]<<8;
					 UInt32 msk = d[3] | d[4]<<8;
					 d += 5;
					 num -= 5;
					 this->richTextBox1->AppendText(String::Format("Standard: {0,8:x} {1,8:x}\n", id, msk));
				 }else{ //extended
					 if( num < 9 ){
						 this->richTextBox1->AppendText("QBridge error in returned extended filter data size");
						 return;
					 }
					 UInt32 id = d[1] | d[2]<<8 | d[3]<<16 | d[4]<<24;
					 UInt32 msk = d[5] | d[6]<<8 | d[7] << 16 | d[8] << 24;
					 d += 9;
					 num -= 9;
					 this->richTextBox1->AppendText(String::Format("Extended: {0,8:x} {1,8:x}\n", id, msk));
				 }
			 }
		 }
	 }
	 else if( cmd == "sf"){ //set filter
		 Byte *list = new Byte[500];
		 list[0] = 'f';	//filter sub command
		 Byte *l = &list[1];
		 int state = 0;
		 for( int i=1; i<args->Length; i++ ){
			 int myval = Int32::Parse(args[i],System::Globalization::NumberStyles::HexNumber);
			 switch( state ) {
				 case 0:	//get the on/off indicator
					 *l++ = myval ? 1 : 0;
					 state = 1;
					 break;
				 case 1:	//get the extended/standard indicator
					 *l++ = myval ? 1 : 0;
					 if( myval ) state = 4; else state = 2;
					 break;
				 case 2:	//get the standard identifier
					 *l++ = myval & 0xff;
					 *l++ = (myval >> 8) & 0xff;
					 state = 3;
					 break;
				 case 3:	//get the standard mask
					 *l++ = myval & 0xff;
					 *l++ = (myval >> 8) & 0xff;
					 state = 1;
					 break;
				 case 4:	//get the extended identifier
					 *l++ = myval & 0xff;
					 *l++ = (myval >> 8) & 0xff;
					 *l++ = (myval >> 16) & 0xff;
					 *l++ = (myval >> 24) & 0xff;
					 state = 5;
					 break;
				 case 5:	//get the extended mask
					 *l++ = myval & 0xff;
					 *l++ = (myval >> 8) & 0xff;
					 *l++ = (myval >> 16) & 0xff;
					 *l++ = (myval >> 24) & 0xff;
					 state = 1;
			 }
		 }
		 for( int x=0; x<l-list; x++ )
			 this->richTextBox1->AppendText(String::Format("{0,2:x} ",list[x]));
		 this->qbridge->sendCANcontrol(l-list,list);
		 delete list;
	 }
	 else if( cmd == "exe" ){
		 unsigned char myd[100];
		 myd[0] = 8;	//parameter size (used by the pj function in this application... not passed to the qbridge)
		 myd[1] = 'x';	//this will be passed to the qbridge as the first parameter
		 myd[2] = 0;
		 myd[3] = 0;
		 myd[4] = 0;
		 int fn = Int32::Parse(args[1],System::Globalization::NumberStyles::HexNumber);
		 myd[5] = fn;
		 myd[6] = (fn>>8) & 0xff;
		 myd[7] = (fn>>16) & 0xff;
		 myd[8] = (fn>>24) & 0xff;
 		 this->qbridge->pj( myd );
	 }
	 else if( cmd == "rw" || cmd == "rh" || cmd == "rb" || cmd == "rs" ){ //read word, half word, byte, string
		 unsigned char myd[100];
		 String ^fs;
		 myd[0] = 0;	//parameter size (used by the pj function in this application... not passed to the qbridge)
		 myd[1] = 'r';	//this will be passed to the qbridge as the first parameter
		 myd[2] = cmd->Substring(1,1)->ToCharArray()[0];
		 myd[3] = 1;	//list type = addr and length
		 myd[4] = 0;	//reserved
		 if( args->Length >= 2 ){
//		 for(;;){	//build up the parameter list
			 int x = Int32::Parse(args[1],System::Globalization::NumberStyles::HexNumber);
			 myd[5] = x & 0xff;
			 myd[6] = (x>>8) & 0xff;
			 myd[7] = (x>>16) & 0xff;
			 myd[8] = (x>>24) & 0xff;
			 if( args->Length >= 3 ){
				int l = Int32::Parse(args[2],System::Globalization::NumberStyles::HexNumber);
				 myd[9] = l & 0xff;
				 myd[10] = (l>>8) & 0xff;
				 myd[11] = (l>>16) & 0xff;
				 myd[12] = (l>>24) & 0xff;
				 myd[0] = 12;
			 }
//		 }
		 }
//		 for( int n=0; n<20; n++ )
//			 this->richTextBox1->AppendText(String::Format("{0,2:x} ", myd[n]));
		 this->qbridge->pj( myd );
		 if( this->qbridge->status == 0 ){
			 int num = this->qbridge->rdata[0];
			 //this->richTextBox1->AppendText("\n");

			 switch( myd[2] ){
				 case 'w': num = num/4; fs="{0,8:x} "; break;
				 case 'h': num = num/2; fs="{0,4:x} "; break;
				 case 'b': num = num/1; fs="{0,2:x} "; break;
				 case 's': num = num/1; fs="{0}"; break;
			 }
			 for( int x=0; x<num; x++ ){
				 UInt32 d;
				 switch( myd[2] ){
					 case 'w': d = this->qbridge->rdata[1+(x*4)+0] | (this->qbridge->rdata[1+(x*4)+1] << 8) | (this->qbridge->rdata[1+(x*4)+2] << 16) | (this->qbridge->rdata[1+(x*4)+3] << 24); break;
					 case 'h': d = this->qbridge->rdata[1+(x*4)+0] | (this->qbridge->rdata[1+(x*4)+1] << 8); break;
					 case 'b': d = this->qbridge->rdata[1+x]; break;
					 case 's': d = this->qbridge->rdata[1+x]; break;
				 }
				 this->richTextBox1->AppendText(String::Format(fs, d));
			 }
			 qbridge->rdata[0] = 0;	//so whoever called us doesn't attempt to display the data
			 this->richTextBox1->AppendText("\n");
		 }
	 }
	 else if (cmd == "ww" || cmd == "wh" || cmd == "wb" || cmd == "ws" ){ //write word, half word, byte, string
		 unsigned char myd[100];
		 myd[0] = 0;	//parameter size (used by the pj function in this application... not passed to the qbridge)
		 myd[1] = 'w';	//this will be passed to the qbridge as the first parameter
		 myd[2] = cmd->Substring(1,1)->ToCharArray()[0];
		 myd[3] = 1;	//list type = addr and length
		 myd[4] = 0;	//reserved
		 if( args->Length <= 2 ) {this->qbridge->status = 0x20; this->qbridge->err = " Bad command";}
		 else{
			 int addr = Int32::Parse(args[1],System::Globalization::NumberStyles::HexNumber);
			 myd[5] = addr & 0xff;
			 myd[6] = (addr>>8) & 0xff;
			 myd[7] = (addr>>16) & 0xff;
			 myd[8] = (addr>>24) & 0xff;
			 unsigned char *mydp = &myd[9];
			 for( int i=2; i< args->Length; i++ ){
				int val = Int32::Parse(args[i],System::Globalization::NumberStyles::HexNumber);
				switch( myd[2] ){
					case 'w':
						 mydp[0] = val & 0xff;
						 mydp[1] = (val>>8) & 0xff;
						 mydp[2] = (val>>16) & 0xff;
						 mydp[3] = (val>>24) & 0xff;
						 mydp += 4;
						 break;
					case 'h':
						 mydp[0] = val & 0xff;
						 mydp[1] = (val>>8) & 0xff;
						 mydp += 2;
						 break;
					case 'b':
						 mydp[0] = val & 0xff;
						 mydp += 1;
						 break;
					case 's':
						 mydp[0] = val & 0xff;
						 mydp += 1;
						 break;

			 }
			 }
			myd[0] = mydp - myd - 1;
			//for( int n=0; n<20; n++ )
			// this->richTextBox1->AppendText(String::Format("{0,2:x} ", myd[n]));
			this->qbridge->pj( myd );
		 }
	 }
	 else {this->qbridge->status = 0x20; this->qbridge->err = " Bad command";}
}

System::UInt32 Form1::readword( UInt32 x ){
	 unsigned char myd[100];
	 myd[0] = 0;	//parameter size (used by the pj function in this application... not passed to the qbridge)
	 myd[1] = 'r';	//this will be passed to the qbridge as the first parameter
	 myd[2] = 'w';
	 myd[3] = 1;	//list type = addr and length
	 myd[4] = 0;	//reserved
	 myd[5] = x & 0xff;			//setup the address
	 myd[6] = (x>>8) & 0xff;	// ||
	 myd[7] = (x>>16) & 0xff;	// ||
	 myd[8] = (x>>24) & 0xff;	// ||
	 int l = 1;
	 myd[9] = l & 0xff;			//setup the length
	 myd[10] = (l>>8) & 0xff;
	 myd[11] = (l>>16) & 0xff;
	 myd[12] = (l>>24) & 0xff;
	 myd[0] = 12;
	 this->qbridge->pj( myd );
	 if( this->qbridge->status == 0 ){
		 int num = this->qbridge->rdata[0];
		 for( int x=0; x<num; x++ ){
			 UInt32 d;
			 switch( myd[2] ){
				 case 'w': d = this->qbridge->rdata[1+(x*4)+0] | (this->qbridge->rdata[1+(x*4)+1] << 8) | (this->qbridge->rdata[1+(x*4)+2] << 16) | (this->qbridge->rdata[1+(x*4)+3] << 24); break;
				 case 'h': d = this->qbridge->rdata[1+(x*4)+0] | (this->qbridge->rdata[1+(x*4)+1] << 8); break;
				 case 'b': d = this->qbridge->rdata[1+x]; break;
				 case 's': d = this->qbridge->rdata[1+x]; break;
			 }
			 return( d );
		 }
	 }
	 throw gcnew System::ArgumentNullException("bad response from qbridge to read command");
	 return(0);
}

System::Void Form1::j1708_info( void ){
	unsigned char data[10];
	data[0] = j1708info;
	this->qbridge->get_info( data );
	if( this->qbridge->rdata[0] != 56 ) throw gcnew System::ArgumentNullException("bad response from qbridge to get info command (j1708 option)");
	//let's display the info returned
	//for( int i=0; i< this->qbridge->rdata[0]; i++ ){
	//	if( (i %16) == 0 ) this->richTextBox1->AppendText("\n");
	//	this->richTextBox1->AppendText(String::Format("{0:X2} ", this->qbridge->rdata[i+1]));
	//}
	this->richTextBox1->AppendText("\n");
	unsigned char *d = &this->qbridge->rdata[1];
#define xd( dptr, var ) do{(var = dptr[0] + (dptr[1]<<8) + (dptr[2]<<16) + (dptr[3]<<24)); dptr+=4;}while(0)
	int tmp1, tmp2;
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("j1708RecvPacketCount    = {0,4}    j1708WaitForBusyBusCount  = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("j1708CollisionCount     = {0,4}    j1708DroppedRxCount       = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("j1708DroppedTxCnfrmCount= {0,4}    j1708DroppedFromHostCount = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("j1708MIDFilterEnabled   = {0,4}    j1708TransmitConfirm      = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	//xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("j1708RxQueueOverflowed  = {0,4}\n", tmp1));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("j1708BusCurState        = {0,4}    j1708BusTransitionDetected= {1,4}\n", tmp2, tmp1));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("FreeJ1708TxBuffers      = {0,4}    FreeJ1708RxBuffers        = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	this->richTextBox1->AppendText(String::Format("j1708_overflow_report   = {0,4}\n", tmp1));
#undef xd
}

System::Void Form1::can_info( void ){
	unsigned char data[10];
	data[0] = caninfo;
	this->qbridge->get_info( data );
	if( this->qbridge->rdata[0] != 120 ) throw gcnew System::ArgumentNullException("bad response from qbridge to get info command (can option)");
	//let's display the info returned
	for( int i=0; i< this->qbridge->rdata[0]; i++ ){
		if( (i %16) == 0 ) this->richTextBox1->AppendText("\n");
		this->richTextBox1->AppendText(String::Format("{0:X2} ", this->qbridge->rdata[i+1]));
	}
	this->richTextBox1->AppendText("\n");
	unsigned char *d = &this->qbridge->rdata[1];
#define xd( dptr, var ) do{(var = dptr[0] + (dptr[1]<<8) + (dptr[2]<<16) + (dptr[3]<<24)); dptr+=4;}while(0)
	int tmp1, tmp2;
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("TxCnt                       = {0,4}    RxCnt                   = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("StuffErrCnt                 = {0,4}    FormErrCnt              = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("AckErrCnt                   = {0,4}    Bit1ErrCnt              = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("Bit0ErrCnt                  = {0,4}    CRCErrCnt               = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("LostMessageCnt              = {0,4}    can_int_queue_overflowed= {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("can_int_queue_overflow_count= {0,4}    CANtransmitConfirm      = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("CANBusOffNotify             = {0,4}    CANautoRestart          = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("CANautRecoverCount          = {0,4}    CANrxWaitForHostCount   = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("CANrxBadValueCount          = {0,4}    boff_int_cnt            = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("boff_notify_cnt             = {0,4}    boff_want_notify_cnt    = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("CANbusErrReportCount        = {0,4}    CANdroppedFromHostCount = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("CANBusCurState              = {0,4}    CANBusTransitionDetected= {1,4}\n", tmp2, tmp1));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("FreeCANtxBuffers            = {0,4}    FreeCANrxBuffers        = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("CANBaud                     = {0,4}    CANTestMode             = {1,4}\n", tmp1, tmp2));
	xd(d, tmp1);
	xd(d, tmp2);
	this->richTextBox1->AppendText(String::Format("CANHWERRCNT                 = {0:x4}   can_overflowed_report   = {1,4}\n", tmp1, tmp2));

}

System::Void Form1::rst_info( void ){
	unsigned char data[10];
	data[0] = rst_ua;
	this->qbridge->get_info( data );
	if( this->qbridge->rdata[0] != 0 ) throw gcnew System::ArgumentNullException("bad response from qbridge to get info command (reset ua option)");
}

System::Void Form1::inq_info( void ) {
	unsigned char data[10];
	data[0] = buildinfo;
	this->qbridge->get_info( data );
	//if( this->qbridge->rdata[0] != ?? ) throw gcnew System::ArgumentNullException("bad response from qbridge to get info command (build info option)");
}