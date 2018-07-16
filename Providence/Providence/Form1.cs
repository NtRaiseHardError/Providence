using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Providence {
    public partial class Form1 : Form {
        ToolTip rulesDirectoryToolTip;
        BackgroundWorker worker;
        bool scannerEnabled;

        public Form1() {
            InitializeComponent();

            rulesDirectoryToolTip = new ToolTip();
            worker = new BackgroundWorker();
            scannerEnabled = true;
        }

        private void disableScannerButton_Click(object sender, EventArgs e) {
            if (scannerEnabled) {
                // Disconnect application from filter port.
                
                disableScannerButton.Text = "Enable Scanner";

                scannerEnabled = false;
            } else {
                // Connect application to filter port.
                
                disableScannerButton.Text = "Disable Scanner";

                scannerEnabled = true;
            }
        }

        private void browseButton_Click(object sender, EventArgs e) {
            // Open dialog.
            FolderBrowserDialog fbd = new FolderBrowserDialog();
            if (fbd.ShowDialog() == DialogResult.OK) {
                // Modify rulesDirectoryTextbox text to selected directory.
                rulesDirectoryTextbox.Text = fbd.SelectedPath;
            }
        }

        private void rulesDirectoryTextbox_MouseHover(object sender, EventArgs e) {
            // Display tool tip of the text in rulesDirectoryTextbox if any.
            if (rulesDirectoryTextbox.Text != "") {
                rulesDirectoryToolTip.SetToolTip(rulesDirectoryTextbox, rulesDirectoryTextbox.Text);
            }
        }

        private void scanFilesButton_Click(object sender, EventArgs e) {
            // Open dialog.
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.CheckFileExists = true;
            ofd.Multiselect = true;
            if (ofd.ShowDialog() == DialogResult.OK) {
                // Select files and add them to a task queue to be scanned in a background worker.

            }
        }
    }
}
