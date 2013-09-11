//******** Version History **********//

//  Ver     Date        Modifier    Description 
//  1.00    10/26/2010  PJ          Convert to VS2008, fix problems so it will actually run correctly
//  1.01    10/07/2011  PJ          Add more COM ports so we can get to the correct COM port for VM
//  1.02    08/13/2012  BED         Added command line support for the firmware upgrader.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;

namespace QBridgeFirmwareUpgrade
{
    public partial class UpdateFWForm : Form
    {
        private Microsoft.Win32.RegistryKey key;
        private string msg; // used for error messages to display in GUI or console

        public UpdateFWForm()
        {
            InitializeComponent();
        }

        private bool use_builtin_version;

        private void UpdateFWForm_Load(object sender, EventArgs e)
        {
            use_builtin_version = true;
            if (use_builtin_version)
            {
                Filename.Text = "qbridge_v1_011b.srec";
                Filename.Enabled = false;
                SerialPort.SelectedIndex = 7;
                SerialPort.Enabled = false;
                BrowseBtn.Visible = false;
                UpgradeBtn.Enabled = false;
                this.Text = "QBridge Firmware Updater - V1.011b";
                this.Left = Screen.PrimaryScreen.Bounds.Width / 2 - this.Width / 2;
                this.Top = Screen.PrimaryScreen.Bounds.Height / 2 - this.Height / 2;
                this.MaximizeBox = false;
                this.MinimizeBox = false;
                this.ControlBox = false;
                this.Show();
                this.Refresh();
                UpgradeBtn_Click(null, null);
                return;
            }

            Microsoft.Win32.RegistryKey topLevel = Microsoft.Win32.Registry.CurrentUser.CreateSubKey("QSI Corporation");
            key = topLevel.CreateSubKey("QBridgeFirmwareUpgrade");
            Filename.Text = (String)(key.GetValue("DefaultFilename", ""));
            SerialPort.SelectedIndex = (int)(key.GetValue("DefaultComPortIndex", 3));
            
            if (Program.ConsoleMode == true)
            {
                HideForm1.Enabled = true;
                Filename.Text = Program.FilePath;
                serPort.PortName = Program.PortName;
                UpgradeBtn_Click(null, null);
            }
            if (Program.AutoRunGUI == true)
            {
                this.Show();
                this.Refresh();
                Filename.Text = Program.FilePath;
                serPort.PortName = Program.PortName;
                UpgradeBtn_Click(null, null);
            }
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

        /**********************************/
        /* Command Line Arguments passed */
        /********************************/
        //private void CmdLineArguments()
        //{
                          
        
        //}

        /*********************/
        /* UpgradeBtn_Click */
        /*******************/
        private void UpgradeBtn_Click(object sender, EventArgs e)
        {
            // Open the file
            byte[] fileContents;
            if (use_builtin_version)
            {
                object obj = Properties.Resources.qbridge_v1_011b;
                fileContents = (byte[])(obj);
            }
            else
            {

                try
                {
                    FileStream fs = new FileStream(Filename.Text, FileMode.Open);
                    fileContents = new byte[fs.Length];
                    fs.Read(fileContents, 0, (int)fs.Length);
                    fs.Close();
                }
                catch (Exception ex)
                {
                    msg = ex.Message + "\nUnable to open or read " + Filename.Text + ".";

                    if (Program.ConsoleMode)
                    {
                        Console.WriteLine(msg);
                        Application.Exit();
                    }
                    else
                        MessageBox.Show(msg, "File error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                    return;

                }

                // Set the registry keys for default file name and COM port index
                key.SetValue("DefaultFilename", Filename.Text);
                if (Program.ConsoleMode)
                    SerialPort.SelectedIndex = Convert.ToInt32(Program.PortName.Remove(0, 3));

                key.SetValue("DefaultComPortIndex", SerialPort.SelectedIndex);
            }


            // Set the progress bar max depending on file size contents
                DLProgress.Minimum = 0;
                DLProgress.Maximum = fileContents.Length - 1;

            // Configure the serial port
            char[] splitChars = { ' ' };
            serPort.PortName = SerialPort.Text.Split(splitChars)[0];
            try
            {
                serPort.Open();
            }
            catch (Exception)
            {
                msg = "Unable to open '" + serPort.PortName + "' for communication.";
                if (Program.ConsoleMode || use_builtin_version)
                {
                    Console.WriteLine(msg);
                    Application.Exit();
                }
                else
                    MessageBox.Show(msg);
                return;
            }

            // Place QBridge in bootloader
            byte[] connectCmd = { 0x02, 0x06, 0x40, 0x03, 0xA7, 0xE6 };
            byte[] resetCmd = { 0x02, 0x06, 0x48, 0x04, 0xE9, 0x1F };

            serPort.Write(connectCmd, 0, connectCmd.GetLength(0));
            serPort.Write(connectCmd, 0, connectCmd.GetLength(0));
            serPort.Write(resetCmd, 0, resetCmd.GetLength(0));
            serPort.Write(resetCmd, 0, resetCmd.GetLength(0));
            System.Threading.Thread.Sleep(100);

            // Verify QBridge in bootloader
            {
                int loopCount;
                byte[] recv = new byte[256];
                int recvCnt = 0;

                FlushPort();
                for (loopCount = 0; loopCount < 30; loopCount++)
                {
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
                    if ((recvCnt > 0) && (recv[0] == 0x15))
                    {
                        break;
                    }
                }
                if (loopCount >= 30)
                {
                    msg = "Device did not enter bootloader as requested\n";
                    // Device did not enter the bootloader

                    if (Program.ConsoleMode || use_builtin_version)
                    {
                        Console.WriteLine(msg);
                        Application.Exit();
                    }
                    else
                        MessageBox.Show(msg, "Device error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);

                    serPort.Close();
                    return;
                }

                FlushPort();
            }

            {
                int fileIndex = 0;
                int recvCnt = 0;
                byte[] recv = new byte[256];
                System.Threading.Thread.Sleep(100);
                for (int nLines = 0; ; nLines++)
                {
                    // Get an S-Record line and write it out - with a newline
                    byte[] line;
                    if (GetLine(out line, ref fileIndex, fileContents) != -1)
                    {
                        break;
                    }

                    recvCnt = 0;
                    for (int retryCount = 0; (recvCnt == 0) && (retryCount < 3); retryCount++)
                    {
                        serPort.Write(line, 0, line.Length);
                        System.Threading.Thread.Sleep(2);

                        // Wait for an ack
                        //for (System.DateTime timeOutTime = System.DateTime.Now.AddMilliseconds(1000); System.DateTime.Now < timeOutTime; ) {
                        for (System.DateTime starttime = System.DateTime.Now; System.DateTime.Now.Subtract(starttime).Milliseconds < 10800; )
                        {
                            try
                            {
                                recvCnt = serPort.Read(recv, 0, recv.GetLength(0));
                            }
                            catch (Exception ex)
                            {
                                //MessageBox.Show(String.Format("exception {1} when writing line {0}.", nLines, ex.Message)); 
                                recvCnt = 0;
                            }
                            if (recvCnt > 0)
                            {
                                int idx;
                                for (idx = 0; idx < recvCnt; idx++)
                                {
                                    if (recv[idx] != 0x06)
                                    {
                                        //TRACE(_T("Expected ack and received <%02x> (byte %d.  Line %d) - retrying %d!\n"), recv[idx], idx, nLines, retryCount);
                                        msg = String.Format("bad bl ack ({1:x} offs={2}, cnt={3}) when writing line {0}.", nLines, recv[idx], idx, recvCnt);

                                        if (Program.ConsoleMode || use_builtin_version)
                                        {
                                            Console.WriteLine(msg);
                                            Application.Exit();
                                        }
                                        else
                                            MessageBox.Show(msg);

                                        recvCnt = 0;
                                        goto ContinueLoop;
                                    }
                                }
                                // all received bytes were ACKS
                                break;
                            }
                            System.Threading.Thread.Sleep(2);
                        }
                    ContinueLoop:
                        ; // NULL statement for the goto
                        if (recvCnt != 0)
                            break;
                    }
                    if (recvCnt == 0)
                    {
                        //TRACE (_T("Device did not respond when writing line %d\n"), nLines);
                        msg = String.Format("Device did not respond when writing line {0} {1}.", nLines, line.Length);
                        if (Program.ConsoleMode || use_builtin_version)
                        {
                            Console.WriteLine(msg);
                            Application.Exit();
                        }
                        else
                            MessageBox.Show(msg);
                        serPort.Close();
                        return;
                    }

                    if (Program.ConsoleMode)
                    {
                        double progress = Math.Round((fileIndex - 1) / (double)DLProgress.Maximum, 3); 
                        double val = (progress* 1000);
                        if (val % 80 == 0)
                        //if (val !=0 && val % 80 == 0)
                        {
                            msg = String.Format("-- Upgrading QBridge firmware {0}% completed....", val/10);
                            Console.WriteLine(msg);
                        }
                    }
                    else
                        DLProgress.Value = fileIndex - 1;
                    //Read the result and compare to the string that was transmitted
                }

                msg = String.Format("Upgrade complete.");
                if (Program.ConsoleMode || use_builtin_version)
                {
                    if (Program.ConsoleMode)
                    {
                        Console.WriteLine("-- Upgrading QBridge firmware 100% completed....");
                        Console.WriteLine(msg);
                    }
                    Application.Exit();
            }
                else
                    MessageBox.Show(msg);
            }
            serPort.Close();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            this.Hide();
            HideForm1.Enabled = false;
        }
    }
}