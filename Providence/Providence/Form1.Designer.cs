namespace Providence {
    partial class Form1 {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            this.disableScannerButton = new System.Windows.Forms.Button();
            this.browseButton = new System.Windows.Forms.Button();
            this.rulesDirectoryLabel = new System.Windows.Forms.Label();
            this.rulesDirectoryTextbox = new System.Windows.Forms.TextBox();
            this.currentTaskLabel = new System.Windows.Forms.Label();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.mainTabPage = new System.Windows.Forms.TabPage();
            this.scanFilesButton = new System.Windows.Forms.Button();
            this.settingsTabPage = new System.Windows.Forms.TabPage();
            this.tabControl1.SuspendLayout();
            this.mainTabPage.SuspendLayout();
            this.settingsTabPage.SuspendLayout();
            this.SuspendLayout();
            // 
            // disableScannerButton
            // 
            this.disableScannerButton.Location = new System.Drawing.Point(7, 49);
            this.disableScannerButton.Name = "disableScannerButton";
            this.disableScannerButton.Size = new System.Drawing.Size(262, 34);
            this.disableScannerButton.TabIndex = 0;
            this.disableScannerButton.Text = "Disable Scanner";
            this.disableScannerButton.UseVisualStyleBackColor = true;
            this.disableScannerButton.Click += new System.EventHandler(this.disableScannerButton_Click);
            // 
            // browseButton
            // 
            this.browseButton.Location = new System.Drawing.Point(191, 16);
            this.browseButton.Name = "browseButton";
            this.browseButton.Size = new System.Drawing.Size(78, 23);
            this.browseButton.TabIndex = 1;
            this.browseButton.Text = "Browse";
            this.browseButton.UseVisualStyleBackColor = true;
            this.browseButton.Click += new System.EventHandler(this.browseButton_Click);
            // 
            // rulesDirectoryLabel
            // 
            this.rulesDirectoryLabel.AutoSize = true;
            this.rulesDirectoryLabel.Location = new System.Drawing.Point(4, 3);
            this.rulesDirectoryLabel.Name = "rulesDirectoryLabel";
            this.rulesDirectoryLabel.Size = new System.Drawing.Size(80, 13);
            this.rulesDirectoryLabel.TabIndex = 2;
            this.rulesDirectoryLabel.Text = "Rules directory:";
            // 
            // rulesDirectoryTextbox
            // 
            this.rulesDirectoryTextbox.Location = new System.Drawing.Point(7, 18);
            this.rulesDirectoryTextbox.Name = "rulesDirectoryTextbox";
            this.rulesDirectoryTextbox.ReadOnly = true;
            this.rulesDirectoryTextbox.Size = new System.Drawing.Size(178, 20);
            this.rulesDirectoryTextbox.TabIndex = 3;
            this.rulesDirectoryTextbox.MouseHover += new System.EventHandler(this.rulesDirectoryTextbox_MouseHover);
            // 
            // currentTaskLabel
            // 
            this.currentTaskLabel.AutoSize = true;
            this.currentTaskLabel.Location = new System.Drawing.Point(4, 3);
            this.currentTaskLabel.Name = "currentTaskLabel";
            this.currentTaskLabel.Size = new System.Drawing.Size(84, 13);
            this.currentTaskLabel.TabIndex = 4;
            this.currentTaskLabel.Text = "No active tasks.";
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.mainTabPage);
            this.tabControl1.Controls.Add(this.settingsTabPage);
            this.tabControl1.Location = new System.Drawing.Point(1, 1);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(283, 115);
            this.tabControl1.TabIndex = 5;
            // 
            // mainTabPage
            // 
            this.mainTabPage.Controls.Add(this.scanFilesButton);
            this.mainTabPage.Controls.Add(this.currentTaskLabel);
            this.mainTabPage.Location = new System.Drawing.Point(4, 22);
            this.mainTabPage.Name = "mainTabPage";
            this.mainTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.mainTabPage.Size = new System.Drawing.Size(275, 89);
            this.mainTabPage.TabIndex = 0;
            this.mainTabPage.Text = "Main";
            this.mainTabPage.UseVisualStyleBackColor = true;
            // 
            // scanFilesButton
            // 
            this.scanFilesButton.Location = new System.Drawing.Point(7, 49);
            this.scanFilesButton.Name = "scanFilesButton";
            this.scanFilesButton.Size = new System.Drawing.Size(262, 34);
            this.scanFilesButton.TabIndex = 5;
            this.scanFilesButton.Text = "Scan Files";
            this.scanFilesButton.UseVisualStyleBackColor = true;
            this.scanFilesButton.Click += new System.EventHandler(this.scanFilesButton_Click);
            // 
            // settingsTabPage
            // 
            this.settingsTabPage.Controls.Add(this.rulesDirectoryLabel);
            this.settingsTabPage.Controls.Add(this.disableScannerButton);
            this.settingsTabPage.Controls.Add(this.browseButton);
            this.settingsTabPage.Controls.Add(this.rulesDirectoryTextbox);
            this.settingsTabPage.Location = new System.Drawing.Point(4, 22);
            this.settingsTabPage.Name = "settingsTabPage";
            this.settingsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.settingsTabPage.Size = new System.Drawing.Size(275, 89);
            this.settingsTabPage.TabIndex = 1;
            this.settingsTabPage.Text = "Settings";
            this.settingsTabPage.UseVisualStyleBackColor = true;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(284, 115);
            this.Controls.Add(this.tabControl1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "Providence Malware Scanner";
            this.tabControl1.ResumeLayout(false);
            this.mainTabPage.ResumeLayout(false);
            this.mainTabPage.PerformLayout();
            this.settingsTabPage.ResumeLayout(false);
            this.settingsTabPage.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button disableScannerButton;
        private System.Windows.Forms.Button browseButton;
        private System.Windows.Forms.Label rulesDirectoryLabel;
        private System.Windows.Forms.TextBox rulesDirectoryTextbox;
        private System.Windows.Forms.Label currentTaskLabel;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage mainTabPage;
        private System.Windows.Forms.TabPage settingsTabPage;
        private System.Windows.Forms.Button scanFilesButton;
    }
}

