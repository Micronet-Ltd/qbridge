namespace TestQBridgeApp
{
    partial class Form1
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
            this.udpTextBox = new System.Windows.Forms.TextBox();
            this.button2 = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.clearTxtBtn = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // udpTextBox
            // 
            this.udpTextBox.Location = new System.Drawing.Point(2, 14);
            this.udpTextBox.Multiline = true;
            this.udpTextBox.Name = "udpTextBox";
            this.udpTextBox.Size = new System.Drawing.Size(500, 511);
            this.udpTextBox.TabIndex = 3;
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(293, 540);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(101, 45);
            this.button2.TabIndex = 4;
            this.button2.Text = "UDP Send String";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(223, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(56, 13);
            this.label2.TabIndex = 5;
            this.label2.Text = "UDP Data";
            // 
            // clearTxtBtn
            // 
            this.clearTxtBtn.Location = new System.Drawing.Point(2, 545);
            this.clearTxtBtn.Name = "clearTxtBtn";
            this.clearTxtBtn.Size = new System.Drawing.Size(79, 35);
            this.clearTxtBtn.TabIndex = 7;
            this.clearTxtBtn.Text = "Clear Text";
            this.clearTxtBtn.UseVisualStyleBackColor = true;
            this.clearTxtBtn.Click += new System.EventHandler(this.clearTxtBtn_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(516, 592);
            this.Controls.Add(this.clearTxtBtn);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.udpTextBox);
            this.Name = "Form1";
            this.Text = "QBridge Emu";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.Form1_FormClosed);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox udpTextBox;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button clearTxtBtn;
    }
}

