using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
//using System.Messaging;
//using System.ServiceModel;
#if forWIN32
#else
using Microsoft.WindowsCE.Forms;
using System.Runtime.InteropServices;
#endif
using System.Windows;
using qbridge_serial_com;

namespace j1708TestCE
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
            mainq = new byte[mainq_size];
            pkt = new byte[pkt_size];
            qbridge = new qbridge_serial();
            qbridge.myf = this;
            MenuItem mi = mainMenu1.MenuItems[0].MenuItems[0];
            for (int i = 0; i < mi.MenuItems.Count; i++)
                if (mi.MenuItems[i].Checked)
                    set_mainSerial(mi.MenuItems[i].Text);

        }
        private qbridge_serial qbridge;

        private void checkBox2_CheckStateChanged(object sender, EventArgs e)
        {

        }
        private void set_mainSerial(String x)
        {
            //String[] s;
            //s = System.IO.Ports.SerialPort.GetPortNames();
            //System.Array.Sort(s);
            if( mainSerial.IsOpen )
                mainSerial.Close();
            if (x == "None")
                return;
            try
            {
                mainSerial.PortName = x;
                mainSerial.Open();
                qbridge.mySerial = mainSerial;
            }
            catch (Exception )
            {
                timer1.Enabled = false;
                timer2.Enabled = false;
                MessageBox.Show(String.Format("sorry, {0} couldn't be opened", x));
                //move check-mark to the 'none' indicator
                MenuItem mi = mainMenu1.MenuItems[0].MenuItems[0];
                for (int i = 0; i < mi.MenuItems.Count; i++)
                {
                    mi.MenuItems[i].Checked = false;
                    if (mi.MenuItems[i].Text == "None")
                        mi.MenuItems[i].Checked = true;
                }
            }
        }

        private void menuItemComPort_Click(object sender, EventArgs e)
        {
            MenuItem mi = (MenuItem) sender;
            for (int i = 0; i < mi.Parent.MenuItems.Count; i++)
                mi.Parent.MenuItems[i].Checked = false;

            mi.Checked = true;
            set_mainSerial(mi.Text);

        }

        private byte[] mainq=null;
        private int mainqe = 0;
        private int mainqs = 0;
        private const int mainq_size = 10000;
        private byte[] pkt = null;
        private int pktl = 0;
        private const int pkt_size = 300;
        public bool isSerialDataAvail()
        {
            return mainqs != mainqe;
        }
        public byte readSerialData()
        {
            byte x = mainq[mainqs];
            mainqs++;
            if (mainqs >= mainq_size)
                mainqs = 0;
            return x;
        }

        private void mainSerial_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
		 	int x = mainSerial.BytesToRead;
            while( x > 0 )
            {
                int num_to_read = mainq_size - mainqe - 1;
                num_to_read = num_to_read > x ? x : num_to_read;
			    mainSerial.Read(mainq,mainqe,num_to_read);
                x -= num_to_read;
                mainqe += num_to_read;
                if (mainqe >= 10000)
                    mainqe = 0;
            }
        }
        private void add_text_to_textbox(TextBox t, String s)
        {
#if forWIN32
            t.Text += s;
            t.SelectionLength = 0;
            t.SelectionStart = textBox1.Text.Length;
            t.ScrollToCaret();
#else
            if (t.Text.Length + s.Length >= 30000)
            {
                t.Text = "";
            }
     
            Message msg = Message.Create(t.Handle, 0xc2, IntPtr.Zero, Marshal.StringToBSTR(s));
            MessageWindow.SendMessage(ref msg);
#endif
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
	        if( mainSerial.IsOpen == false ) return;
            if ((qbridge != null) && qbridge.waiting_for_ack) return;
	        //if( mainSerial.BytesToRead != 0 )
            //    mainSerial_DataReceived();

	        int myql = (mainqe >= mainqs) ? mainqe-mainqs : mainqe+mainq_size - mainqs;
	        if( myql > 2 ){	//enough data that we can look for start of message and length?
		        if( mainq[mainqs] == 0x02 ){
			        int l = mainq[(mainqs+1) % mainq_size ];
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
					        char[] n = new char[1];
					        n[0] = (char)mainq[mainqs];
                            String x = new String(n, 0, 1);
					        //this.textBox1.SelectionColor = Color.Blue;
                            add_text_to_textbox(textBox1, x);//textBox1.Text += x;
					        //this.textBox1.SelectionColor = Color.Black;
				        }
				        mainqs = mainqs+1;//skip current start of message
				        if( mainqs >= mainq_size ) mainqs = 0;
			        }
		        }
                //textBox1.SelectionLength = 0;
                //textBox1.SelectionStart = textBox1.Text.Length;
                //textBox1.ScrollToCaret();
	        }

        }
        private void dequemain( )
        {
	        int l = mainq[(mainqs+1) % mainq_size ];
            if (l > pkt_size)
            {
                //don't over-run size allocated for this little guy
                add_text_to_textbox(textBox1, "packet too large... skipping\r\n");//textBox1.Text += "packet too large... skipping\r\n";
                mainqs += l % mainq_size;
            }
	        mainqs = (mainqs + 2) % mainq_size;
	        for( int i=0; i<l-2; i++ ){
		        pkt[i] = mainq[mainqs];
		        mainqs++;
		        if( mainqs >= mainq_size ) mainqs=0;
	        }
	        pktl = l-2;
        }
        private void handle_packet( )
        {
            if (pktl < 6)
            {
                //textBox1.Text += "invalid packet recieved (len < 6)\r\n";
                add_text_to_textbox(textBox1, "invalid packet recieved (len < 6)\r\n");
                return; //this is an invalid packet so ignore it
            }
	        //really should check CRC, but we're not going to...
	        byte id = pkt[1];
	        switch( (char)pkt[0] ){
		        case 'K': //received CAN message
			        do_can_receive( id, ref pkt, 2, pktl-4 );
                    //textBox1.Text += "CAN message\r\n";
                    break;
		        case 'G': //transmit confirm
			        do_transmit_confirm( id, ref pkt, 2, pktl-4);
                    //textBox1.Text += "tranmit confirm\r\n";
                    break;
		        case 'E': //received J1708 message
			        do_j1708_receive( id, ref pkt, 2, pktl-4 );
                    //textBox1.Text += "J1708 message\r\n";
                    break;
		        default:
                    byte[] junk = new byte[2];
			        qbridge.ack(id,ack_enums.ack_invalid_command,ref junk);
			        //textBox1.Text += "bad packet received\r\n";
                    add_text_to_textbox(textBox1, "bad packet received\r\n");
                    break;
	        }
        }
        private void do_can_receive( Byte id, ref byte[] data, int start, int datalen )
        {
	        int l = datalen;
	        if ( (l < 3) //this option requires at least a type identifier (1) and a CAN identifier (2 for the 11 bit version)
	          || (l > 13)//this option can have no more than a type id(1), CAN id (4), and 8 bytes of data
	          || (data[start] > 1) //identifier type can only be 0 or 1
	          || ((data[start] == 1) && (l < 5)) //extended CAN requires 4 byte identifier
	          || ((data[start] == 0) && (l > 11))//standard CAN, id=2, data max=8
	          ){
                  byte[] junk = new byte[2];
		        qbridge.ack (id, ack_enums.ack_invalid_data, ref junk);
		        //textBox1.Text += "bad CAN received data format\r\n";
                add_text_to_textbox(textBox1,"bad CAN received data format\r\n");
                return;
	        }

	        int identifier;
	        int dptr;
            if( data[start] == 0 ){ //standard CAN identifier
                identifier = data[start+1] | (data[start+2] << 8);
                dptr = 3; //&data[3];
                l = l - 3;
            }else{ //extended CAN identifier
                identifier = data[start+1] | (data[start+2] << 8) | (data[start+3] << 16) | (data[start+4] << 24);
                dptr = 5; //&data[5];
                l = l - 5;
            }
            byte[] junk2 = new byte[2];
	        qbridge.ack(id, ack_enums.ack_ok, ref junk2);
	        //textBox1.Text += String.Format("Received CAN data (id={0,8:x} data len = {1} ", identifier, l);
            add_text_to_textbox(textBox1, String.Format("Received CAN data (id={0,8:x} data len = {1} ", identifier, l));
            if (l > 0)
            {
		        //textBox1.Text += "Data: ";
                add_text_to_textbox(textBox1, "Data: ");
                while (l-- > 0)
                {
			        //textBox1.Text += String.Format("{0,2:x}", data[start+dptr++]);
                    add_text_to_textbox(textBox1, String.Format("{0,2:x}", data[start+dptr++]));
                }
	        }
	        //textBox1.Text += ")\r\n";
            add_text_to_textbox(textBox1, ")\r\n");
        }
        private void do_transmit_confirm( Byte id, ref byte[] data, int start, int datalen )
        {
	        int l = datalen;
	        if(l != 5){ //this option has success/failure flag, and id being confirmed
                byte[] junk = new byte[2];
		        qbridge.ack (id, ack_enums.ack_invalid_data, ref junk);
		        //textBox1.Text += "bad transmit confirm data format\r\n";
                add_text_to_textbox(textBox1, "bad transmit confirm data format\r\n");
                return;
	        }
	        //may someday want to keep track of outstanding ids and if this isn't one... generate an error
            byte[] junk2 = new byte[2];
	        qbridge.ack(id, ack_enums.ack_ok, ref junk2);
	        int identifier = data[start+1] | (data[start+2] << 8) | (data[start+3] << 16) | (data[start+4] << 24);
	        //textBox1.Text += String.Format("Received transmit confirmation for id={0,8:x}\r\n", identifier);
            add_text_to_textbox(textBox1, String.Format("Received transmit confirmation for id={0,8:x}\r\n", identifier));
        }
        private void do_j1708_receive( Byte id, ref byte[] data, int start, int datalen )
        {
            byte[] junk = new byte[2];
	        qbridge.ack(id, ack_enums.ack_ok, ref junk);
	        //textBox1.Text += "Received J1708 data: ";
            add_text_to_textbox(textBox1, "Received J1708 data: ");
            if (datalen > 0)
            {
                int dataptr = 0;
		        while( datalen-- != 0){
                    //textBox1.Text += String.Format(" {0,2:x}", data[start+dataptr++]);
                    add_text_to_textbox(textBox1, String.Format(" {0,2:x}", data[start+dataptr++]));
                }
	        }
	        //textBox1.Text += "\r\n";
            add_text_to_textbox(textBox1, "\r\n");
        }

        private void button3_Click(object sender, EventArgs e)
        {
            ddcmd("inq");
        }

        private void button2_Click(object sender, EventArgs e)
        {
            ddcmd("jtx");
        }

        private void button1_Click(object sender, EventArgs e)
        {
            ddcmd("ctx");
        }
        private void can_filter_CheckStateChanged(object sender, EventArgs e)
        {
            CheckBox s = (CheckBox)sender;
            if( s.Checked )
                ddcmd("ena_cfilt");
            else
                ddcmd("dis_cfilt");
        }
        private void j1708_filter_CheckStateChanged(object sender, EventArgs e)
        {
            CheckBox s = (CheckBox)sender;
            if (s.Checked)
                ddcmd("ena_jfilt");
            else
                ddcmd("dis_jfilt");
        }

        private void ddcmd(String cmdline)
        {
            dcmd( cmdline );
            if (qbridge.status != 0)
            {
                if (qbridge.err == null)
                {
                    //textBox1.Text += "Error unknown";
                    add_text_to_textbox(textBox1, "Error unknown");
                }
                else
                {
                    //textBox1.Text += qbridge.err;
                    add_text_to_textbox(textBox1, qbridge.err);
                }
                //textBox1.Text += "\r\n";
                //add_text_to_textbox(textBox1,"\r");
            }
            else
            {
                //textBox1.Text += "good cmd status\r\n";
            }
            //display the returned data from the command.... problem pj... the returned data may or may not be text
            if (qbridge.rdata[0] != 0)
            {
                if (qbridge.rdata[0] == 0xff)
                    //textBox1.Text += qbridge.ret_info;
                    add_text_to_textbox(textBox1, qbridge.ret_info);
                else
                {
                    char[] ca = new char[(qbridge.rdata[0] & 0xff) + 3];
                    int k;
                    for (k = 0; k < qbridge.rdata[0]; k++)
                        ca[k] = (char)qbridge.rdata[1 + k];
                    ca[k] = '\0';
                    String t = new String(ca);
                    //textBox1.Text += t;
                    add_text_to_textbox(textBox1, t);
                }
                //textBox1.Text += "\r\n";
                add_text_to_textbox(textBox1, "\r\n");
            }
            else
            {
                //textBox1.Text += "no data returned from command\r\n";
            }

            //textBox1.SelectionLength = 0;
            //textBox1.SelectionStart = textBox1.Text.Length;
            //textBox1.ScrollToCaret();
        }
        private void dcmd(String cmdline)
        {
            String cmd;
            qbridge.status = 0;
            qbridge.err = "";
            qbridge.rdata[0] = 0;

            cmdline = cmdline.Trim();
            String tmpstr = " \t,(){}[]";
            char[] delims = tmpstr.ToCharArray();
            String[] args = cmdline.Split(delims);
            cmd = args[0];

            cmd = cmd.ToLower();
            if (cmd.Length == 0) return;

            if (cmd == "inq") {
                qbridge.info_request_to_debug_port(debuginfotype.general);
            }else if (cmd == "ctx"){
                byte[] d = new byte[] { (byte)'s', (byte)'a', (byte)'v', (byte)'e', (byte)' ', (byte)'m', (byte)'e' };
                qbridge.sendCANpacket(0x1555555, d.Length, ref d);
            }else if (cmd == "jtx"){
                byte[] d = new byte[] { 8, 111, (byte)'h', (byte)'e', (byte)'l', (byte)'p', (byte)' ', (byte)'m', (byte)'e' };
                qbridge.send1708_packet((byte)d.Length, ref d);
            }else if( cmd == "dis_filt" || cmd == "dis_cfilt"){
                byte[] d = new byte[] { (byte)'e', 0, 0 };
                qbridge.sendCANcontrol(2,ref d);
            }else if( cmd == "ena_filt" || cmd == "ena_cfilt"){
                byte[] d = new byte[] { (byte)'e', 1, 0 };
                qbridge.sendCANcontrol(2,ref d);
            }else if( cmd == "dis_jfilt" ){
                qbridge.mid_filter_enable(false);
            }else if( cmd == "ena_jfilt" ){
                qbridge.mid_filter_enable(true);
            }else{
                //bad command
            }
        }

        private void timer2_Tick(object sender, EventArgs e)
        {
            if (timer2.Interval != 333)
            {
                menuItemComPort_Click(menuItem5,null);
                button3_Click(sender, e);
                timer2.Interval = 333;
            }
            else
            {
                button1_Click(sender, e);
                button2_Click(sender, e);
            }
        }

        private void AutoRun_CheckStateChanged(object sender, EventArgs e)
        {
            if (!AutoRun.Checked)
                timer2.Enabled = false;
            else
                timer2.Enabled = true;
        }

        
    }
}