namespace QBridgeFirmwareUpgrade
{
    partial class UpdateFWForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

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
            this.MyLBL = new System.Windows.Forms.Label();
            this.Filename = new System.Windows.Forms.TextBox();
            this.BrowseBtn = new System.Windows.Forms.Button();
            this.ComPortSerial = new System.Windows.Forms.Label();
            this.SerialPort = new System.Windows.Forms.ComboBox();
            this.UpgradeBtn = new System.Windows.Forms.Button();
            this.DLProgress = new System.Windows.Forms.ProgressBar();
            this.ofd = new System.Windows.Forms.OpenFileDialog();
            this.serPort = new System.IO.Ports.SerialPort(this.components);
            this.SuspendLayout();
            // 
            // MyLBL
            // 
            this.MyLBL.Location = new System.Drawing.Point(4, 13);
            this.MyLBL.Name = "MyLBL";
            this.MyLBL.Size = new System.Drawing.Size(67, 22);
            this.MyLBL.Text = "Filename";
            // 
            // Filename
            // 
            this.Filename.Location = new System.Drawing.Point(75, 12);
            this.Filename.Name = "Filename";
            this.Filename.Size = new System.Drawing.Size(302, 23);
            this.Filename.TabIndex = 1;
            // 
            // BrowseBtn
            // 
            this.BrowseBtn.Location = new System.Drawing.Point(383, 12);
            this.BrowseBtn.Name = "BrowseBtn";
            this.BrowseBtn.Size = new System.Drawing.Size(81, 23);
            this.BrowseBtn.TabIndex = 2;
            this.BrowseBtn.Text = "Browse ...";
            this.BrowseBtn.Click += new System.EventHandler(this.BrowseBtn_Click);
            // 
            // ComPortSerial
            // 
            this.ComPortSerial.Location = new System.Drawing.Point(4, 54);
            this.ComPortSerial.Name = "ComPortSerial";
            this.ComPortSerial.Size = new System.Drawing.Size(74, 24);
            this.ComPortSerial.Text = "Serial port.";
            // 
            // SerialPort
            // 
            this.SerialPort.Items.Add("COM0");
            this.SerialPort.Items.Add("COM1");
            this.SerialPort.Items.Add("COM2");
            this.SerialPort.Items.Add("COM3 (M4x internal QBridge)");
            this.SerialPort.Items.Add("COM4");
            this.SerialPort.Items.Add("COM5");
            this.SerialPort.Items.Add("COM6");
            this.SerialPort.Items.Add("COM7 (VM internal QBridge)");
            this.SerialPort.Items.Add("COM8");
            this.SerialPort.Items.Add("COM9");
            this.SerialPort.Location = new System.Drawing.Point(75, 54);
            this.SerialPort.Name = "SerialPort";
            this.SerialPort.Size = new System.Drawing.Size(229, 23);
            this.SerialPort.TabIndex = 4;
            // 
            // UpgradeBtn
            // 
            this.UpgradeBtn.Location = new System.Drawing.Point(383, 54);
            this.UpgradeBtn.Name = "UpgradeBtn";
            this.UpgradeBtn.Size = new System.Drawing.Size(81, 22);
            this.UpgradeBtn.TabIndex = 5;
            this.UpgradeBtn.Text = "Upgrade";
            this.UpgradeBtn.Click += new System.EventHandler(this.UpgradeBtn_Click);
            // 
            // DLProgress
            // 
            this.DLProgress.Location = new System.Drawing.Point(4, 99);
            this.DLProgress.Name = "DLProgress";
            this.DLProgress.Size = new System.Drawing.Size(460, 23);
            // 
            // ofd
            // 
            this.ofd.Filter = "Firmware files (*.srec)|*.srec|All Files|*.*";
            // 
            // serPort
            // 
            this.serPort.BaudRate = 115200;
            this.serPort.ReadTimeout = 0;
            // 
            // UpdateFWForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(478, 135);
            this.Controls.Add(this.DLProgress);
            this.Controls.Add(this.UpgradeBtn);
            this.Controls.Add(this.SerialPort);
            this.Controls.Add(this.ComPortSerial);
            this.Controls.Add(this.BrowseBtn);
            this.Controls.Add(this.Filename);
            this.Controls.Add(this.MyLBL);
            this.Location = new System.Drawing.Point(0, 56);
            this.Name = "UpdateFWForm";
            this.Text = "Update QBridge Firmware";
            this.Load += new System.EventHandler(this.UpdateFWForm_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label MyLBL;
        private System.Windows.Forms.TextBox Filename;
        private System.Windows.Forms.Button BrowseBtn;
        private System.Windows.Forms.Label ComPortSerial;
        private System.Windows.Forms.ComboBox SerialPort;
        private System.Windows.Forms.Button UpgradeBtn;
        private System.Windows.Forms.ProgressBar DLProgress;
        private System.Windows.Forms.OpenFileDialog ofd;
        private System.IO.Ports.SerialPort serPort;
    }
}

