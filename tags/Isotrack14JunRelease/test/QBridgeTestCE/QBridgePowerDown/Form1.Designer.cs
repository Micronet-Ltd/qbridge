namespace j1708TestCE
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.MainMenu mainMenu1;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.mainMenu1 = new System.Windows.Forms.MainMenu();
            this.menuItem1 = new System.Windows.Forms.MenuItem();
            this.menuItem2 = new System.Windows.Forms.MenuItem();
            this.menuItem3 = new System.Windows.Forms.MenuItem();
            this.menuItem4 = new System.Windows.Forms.MenuItem();
            this.menuItem5 = new System.Windows.Forms.MenuItem();
            this.menuItem6 = new System.Windows.Forms.MenuItem();
            this.menuItem7 = new System.Windows.Forms.MenuItem();
            this.menuItem8 = new System.Windows.Forms.MenuItem();
            this.menuItem9 = new System.Windows.Forms.MenuItem();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.mainSerial = new System.IO.Ports.SerialPort(this.components);
            this.timer1 = new System.Windows.Forms.Timer();
            this.button3 = new System.Windows.Forms.Button();
            this.timer2 = new System.Windows.Forms.Timer();
            this.PowerDownButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // mainMenu1
            // 
            this.mainMenu1.MenuItems.Add(this.menuItem1);
            // 
            // menuItem1
            // 
            this.menuItem1.MenuItems.Add(this.menuItem2);
            this.menuItem1.Text = "Setup";
            // 
            // menuItem2
            // 
            this.menuItem2.MenuItems.Add(this.menuItem3);
            this.menuItem2.MenuItems.Add(this.menuItem4);
            this.menuItem2.MenuItems.Add(this.menuItem5);
            this.menuItem2.MenuItems.Add(this.menuItem6);
            this.menuItem2.MenuItems.Add(this.menuItem7);
            this.menuItem2.MenuItems.Add(this.menuItem8);
            this.menuItem2.MenuItems.Add(this.menuItem9);
            this.menuItem2.Text = "Serial Port";
            // 
            // menuItem3
            // 
            this.menuItem3.Text = "COM1";
            this.menuItem3.Click += new System.EventHandler(this.menuItemComPort_Click);
            // 
            // menuItem4
            // 
            this.menuItem4.Text = "COM2";
            this.menuItem4.Click += new System.EventHandler(this.menuItemComPort_Click);
            // 
            // menuItem5
            // 
            this.menuItem5.Checked = true;
            this.menuItem5.Text = "COM3";
            this.menuItem5.Click += new System.EventHandler(this.menuItemComPort_Click);
            // 
            // menuItem6
            // 
            this.menuItem6.Text = "COM4";
            this.menuItem6.Click += new System.EventHandler(this.menuItemComPort_Click);
            // 
            // menuItem7
            // 
            this.menuItem7.Text = "COM5";
            this.menuItem7.Click += new System.EventHandler(this.menuItemComPort_Click);
            // 
            // menuItem8
            // 
            this.menuItem8.Text = "COM6";
            this.menuItem8.Click += new System.EventHandler(this.menuItemComPort_Click);
            // 
            // menuItem9
            // 
            this.menuItem9.Text = "None";
            this.menuItem9.Click += new System.EventHandler(this.menuItemComPort_Click);
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(3, 29);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(472, 133);
            this.textBox1.TabIndex = 0;
            // 
            // mainSerial
            // 
            this.mainSerial.BaudRate = 115200;
            this.mainSerial.DtrEnable = true;
            this.mainSerial.PortName = "Com3";
            this.mainSerial.RtsEnable = true;
            this.mainSerial.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.mainSerial_DataReceived);
            // 
            // timer1
            // 
            this.timer1.Enabled = true;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(322, 168);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(52, 20);
            this.button3.TabIndex = 5;
            this.button3.Text = "Inq";
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // timer2
            // 
            this.timer2.Enabled = true;
            this.timer2.Interval = 1500;
            // 
            // PowerDownButton
            // 
            this.PowerDownButton.Location = new System.Drawing.Point(3, 168);
            this.PowerDownButton.Name = "PowerDownButton";
            this.PowerDownButton.Size = new System.Drawing.Size(194, 45);
            this.PowerDownButton.TabIndex = 6;
            this.PowerDownButton.Text = "Power Down QBridge Device";
            this.PowerDownButton.Click += new System.EventHandler(this.PowerDownButton_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(478, 247);
            this.Controls.Add(this.PowerDownButton);
            this.Controls.Add(this.button3);
            this.Controls.Add(this.textBox1);
            this.Menu = this.mainMenu1;
            this.Name = "Form1";
            this.Text = "QBridgePowerDownCE";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.MenuItem menuItem1;
        private System.Windows.Forms.MenuItem menuItem2;
        private System.Windows.Forms.TextBox textBox1;
        private System.IO.Ports.SerialPort mainSerial;
        private System.Windows.Forms.MenuItem menuItem3;
        private System.Windows.Forms.MenuItem menuItem4;
        private System.Windows.Forms.MenuItem menuItem5;
        private System.Windows.Forms.MenuItem menuItem6;
        private System.Windows.Forms.MenuItem menuItem7;
        private System.Windows.Forms.MenuItem menuItem8;
        private System.Windows.Forms.MenuItem menuItem9;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Timer timer2;
        private System.Windows.Forms.Button PowerDownButton;
    }
}

