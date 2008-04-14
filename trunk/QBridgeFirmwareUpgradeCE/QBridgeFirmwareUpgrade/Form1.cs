using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace QBridgeFirmwareUpgrade
{
    public partial class UpdateFWForm : Form
    {
        private Microsoft.Win32.RegistryKey key;

        public UpdateFWForm()
        {
            InitializeComponent();
        }

        private void UpdateFWForm_Load(object sender, EventArgs e)
        {
            Microsoft.Win32.RegistryKey topLevel = Microsoft.Win32.Registry.CurrentUser.CreateSubKey("QSI Corporation");
            key = topLevel.CreateSubKey("QBridgeFirmwareUpgrade");
            Filename.Text = (String)(key.GetValue("DefaultFilename", ""));
            SerialPort.SelectedIndex = (int)(key.GetValue("DefaultComPortIndex", 3));
        }

        private void BrowseBtn_Click(object sender, EventArgs e)
        {
            if (ofd.ShowDialog() == DialogResult.OK) {
                Filename.Text = ofd.FileName;
            }
        }

/**************/
/* FlushPort */
/************/
        private void FlushPort() {
            // Now read until there is no more data comming from the device
            byte[] recv = new byte[256];
            do {
                try
                {
                    serPort.Read(recv, 0, recv.GetLength(0));
                }
                catch (TimeoutException)
                {
                    break;
                }
                catch (Exception) {
                }
            } while (true);
        }

/************/
/* GetLine */
/**********/
        private int GetLine(out byte[] line, ref int index, byte[] file) {
            int i;
            // skip whitespace
            for (i = index; i < file.Length; i++) {
                if ((file[i] != ' ') && (file[i] != '\t')) {
                    break;
                }
            }

            int startIdx = i;
            for (; i < file.Length; i++) {
                if (file[i] == '\r') {
                    continue;
                } else if (file[i] == '\n') {
                    // found the line.  Assemble it now.
                    line = new byte[i-startIdx+1];
                    int j, k;
                    for (j = startIdx, k = 0; j <= i; j++, k++) {
                        line[k] = file[j];
                    }
                    index = i+1;
                    return -1;
                } 
            }
            line = null;
            index = i;
            return -2;
        }

/*********************/
/* UpgradeBtn_Click */
/*******************/
        private void UpgradeBtn_Click(object sender, EventArgs e)
        {
            // Open the file
            byte[] fileContents;
            try
            {
                FileStream fs = new FileStream(Filename.Text, FileMode.Open);
                fileContents = new byte[fs.Length];
                fs.Read(fileContents, 0, (int)fs.Length);
                fs.Close();
            }
            catch (Exception )
            {
                MessageBox.Show("Unable to open or read " + Filename.Text + ".", "File error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                return;
            }

            DLProgress.Minimum = 0;
            DLProgress.Maximum = fileContents.Length-1;
            key.SetValue("DefaultFilename", Filename.Text);
            key.SetValue("DefaultComPortIndex", SerialPort.SelectedIndex);

            // Configure the serial port
            char[] splitChars = { ' ' };
            serPort.PortName = SerialPort.Text.Split(splitChars)[0];
            try
            {
                serPort.Open();
            }
            catch (Exception)
            {
                MessageBox.Show("Unable to open '" + serPort.PortName + "' for communication.");
                return;
            }
            
            // Place QBridge in bootloader
            byte[] connectCmd = { 0x02, 0x06, 0x40, 0x03, 0xA7, 0xE6 };
            byte[] resetCmd = { 0x02, 0x06, 0x48, 0x04, 0xE9, 0x1F };

            serPort.Write( connectCmd, 0, connectCmd.GetLength(0) );
            serPort.Write( connectCmd, 0, connectCmd.GetLength(0) );
            serPort.Write(resetCmd, 0, resetCmd.GetLength(0));
            serPort.Write(resetCmd, 0, resetCmd.GetLength(0));
            System.Threading.Thread.Sleep(100);

            // Verify QBridge in bootloader
            {
	            int loopCount;
	            byte [] recv = new byte[256];
	            int recvCnt = 0;

                FlushPort();
	            for (loopCount = 0; loopCount < 30; loopCount++) {
                    byte[] tx = { 0x41, 0x0d, 0x0a };
		            serPort.Write(tx, 0, tx.GetLength(0));
                    System.Threading.Thread.Sleep(100);

                    try
                    {
                        recvCnt = serPort.Read(recv, 0, recv.GetLength(0));
                    }
                    catch (Exception)
                    {
                        recvCnt = 0;
                    }
		            if ((recvCnt > 0) && (recv[0] == '\x15')) {
			            break;
		            }
	            }
	            if (loopCount >= 30) {
		            // Device did not enter the bootloader
                    MessageBox.Show ("Device did not enter bootloader as requested\n", "Device error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                    serPort.Close();
		            return;
	            }
                   
                FlushPort();
            }

            {
                int fileIndex = 0;
                int recvCnt = 0;
                byte [] recv = new byte[256];
	            for (int nLines = 0;; nLines++) {
		            // Get an S-Record line and write it out - with a newline
                    byte[] line;
		            if (GetLine(out line, ref fileIndex, fileContents) != -1) {
			            break;
		            }
            		
		            recvCnt = 0;
		            for (int retryCount = 0; (recvCnt == 0) && (retryCount < 3) ; retryCount++) {
                        serPort.Write(line, 0, line.Length);
            			
			            // Wait for an ack
                        for (System.DateTime timeOutTime = System.DateTime.Now.AddMilliseconds(1000); System.DateTime.Now < timeOutTime; ) {
                            try
                            {
                                recvCnt = serPort.Read(recv, 0, recv.GetLength(0));
                            }
                            catch (Exception)
                            {
                                recvCnt = 0;
                            }
				            if (recvCnt > 0) {
					            int idx;
					            for (idx = 0; idx < recvCnt; idx++) {
						            if (recv[idx] != '\x06') {
							            //TRACE(_T("Expected ack and received <%02x> (byte %d.  Line %d) - retrying %d!\n"), recv[idx], idx, nLines, retryCount);
							            recvCnt = 0;
							            goto ContinueLoop;
						            }
					            }
					            // all received bytes were ACKS
					            break; 
				            }
				            System.Threading.Thread.Sleep(1);
			            }
            ContinueLoop:
			            ; // NULL statement for the goto
		            }
		            if (recvCnt == 0) {
			            //TRACE (_T("Device did not respond when writing line %d\n"), nLines);
                        MessageBox.Show(String.Format("Device did not respond when writing line {0}.", nLines));
                        serPort.Close();
                        return;
		            }
            		DLProgress.Value = fileIndex;
		            //Read the result and compare to the string that was transmitted
	            }
            }
            serPort.Close();
        }
    }
}