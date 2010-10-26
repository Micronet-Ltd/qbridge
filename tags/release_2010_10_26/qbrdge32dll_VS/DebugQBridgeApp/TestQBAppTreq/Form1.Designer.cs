namespace TestQBAppTreq
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
            this.mainMenu1 = new System.Windows.Forms.MainMenu();
            this.clearTxtBtn = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.udpTextBox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // clearTxtBtn
            // 
            this.clearTxtBtn.Location = new System.Drawing.Point(3, 278);
            this.clearTxtBtn.Name = "clearTxtBtn";
            this.clearTxtBtn.Size = new System.Drawing.Size(79, 35);
            this.clearTxtBtn.TabIndex = 10;
            this.clearTxtBtn.Text = "Clear Text";
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(472, 268);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(98, 45);
            this.button2.TabIndex = 9;
            this.button2.Text = "UDP Send String";
            // 
            // udpTextBox
            // 
            this.udpTextBox.Location = new System.Drawing.Point(3, 12);
            this.udpTextBox.Multiline = true;
            this.udpTextBox.Name = "udpTextBox";
            this.udpTextBox.Size = new System.Drawing.Size(567, 804);
            this.udpTextBox.TabIndex = 8;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(575, 326);
            this.Controls.Add(this.clearTxtBtn);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.udpTextBox);
            this.Menu = this.mainMenu1;
            this.Name = "Form1";
            this.Text = "Form1";
            this.Closed += new System.EventHandler(this.Form1_Closed);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button clearTxtBtn;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.TextBox udpTextBox;
    }
}

