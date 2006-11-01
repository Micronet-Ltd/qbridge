using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net.Sockets;
using System.Threading;
using System.Diagnostics;
using System.Net;

namespace TestQBridgeApp
{
    public partial class Form1 : Form
    {
        public const int sendPort = 11234;
        public static UdpClient udpListener;
        private static Thread udpThread;

        public static TextBox udpBox;

        delegate void SetTextCallback(string text, TextBox textbox);

        public static Form1 mainF;


        //debug upd listen
        public const int debugPort = 23233;
        public static UdpClient udpDebug;
        private static Thread udpDbgThread;

        public Form1()
        {
            InitializeComponent();

            udpBox = udpTextBox;

            mainF = this;
            udpThread = new Thread(new ThreadStart(UdpListen));
            udpThread.Start();

            udpDbgThread = new Thread(new ThreadStart(UdpDbgListen));
            udpDbgThread.Start();
        }

        public static void UdpListen()
        {
            // setup udp listener
            int listenPort = (new Random()).Next(1000, 15000);
            udpListener = new UdpClient(listenPort);
            udpListener.Client.Blocking = true;
            IPEndPoint iep = new IPEndPoint(IPAddress.Any, 0);
            byte[] data;
            while (true)
            {
                try
                {
                    data = udpListener.Receive(ref iep);
                    string newdata = (new Helper()).ByteArrayToString(data);
                    mainF.AddText(newdata, udpBox);
                }
                catch (Exception exp)
                { }
            }
        }

        public static void UdpDbgListen()
        {
            // setup udp listener
            udpDebug = new UdpClient(debugPort);
            udpDebug.Client.Blocking = true;
            IPEndPoint iep = new IPEndPoint(IPAddress.Any, 0);
            byte[] data;
            while (true)
            {
                try
                {
                    data = udpDebug.Receive(ref iep);
                    string newdata = (new Helper()).ByteArrayToString(data);
                    mainF.AddText(newdata, udpBox);
                }
                catch (Exception exp)
                { }
            }
        }

        private void AddText(string text, TextBox textbox)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (textbox.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(AddText);
                this.Invoke(d, new object[] { text, textbox });
            }
            else
            {
                textbox.AppendText(text);
            }
        }


        private void button2_Click(object sender, EventArgs e)
        {
            byte[] outData = (new Helper()).StringToByteArray("Abcdef323");
            udpListener.Send(outData, outData.Length, "127.0.0.1", sendPort);
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            udpListener.Client.Blocking = false;
            udpListener.Close();
            udpThread.Abort();
            udpDebug.Close();
            udpDbgThread.Abort();
        }

        private void clearTxtBtn_Click(object sender, EventArgs e)
        {
            udpTextBox.Clear();
        }
    }

    class Helper
    {
        public byte[] StringToByteArray(string source)
        {
            byte[] dest = new byte[source.Length];
            for (int i = 0; i < dest.Length; i++)
            {
                dest[i] = (byte)source[i];
            }
            return dest;
        }

        public string ByteArrayToString(byte[] source)
        {
            char[] tmp = new char[source.Length];
            for (int i = 0; i < tmp.Length; i++)
            {
                tmp[i] = (char)source[i];
            }
            string dest = new String(tmp);
            return dest;
        }
    }
}