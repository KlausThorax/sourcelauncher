using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using ICSharpCode.SharpZipLib.BZip2;
using System.Xml;
using System.Net;
using System.Runtime.InteropServices;
using System.Net.NetworkInformation;
using System.Diagnostics;

namespace PWRLauncher
{
    public partial class Form1 : Form
    {
        //var exists = System.Diagnostics.Process.GetProcessesByName(System.IO.Path.GetFileNameWithoutExtension(System.Reflection.Assembly.GetEntryAssembly().Location)).Count() > 1;
        //if (System.Diagnostics.Process.GetProcessesByName(System.IO.Path.GetFileNameWithoutExtension(System.Reflection.Assembly.GetEntryAssembly().Location)).Count() > 1) return;


        DataTable downloads_info;

        FileInfo fileToBeZipped;
        FileInfo zipFileName;

        static string main_url = "http://patchserver.acorav.de/cabalpatch/";
        static string file_url = main_url + "files.xml";

        static string facebook_url = "https://www.facebook.com/PwrGamesCommunity";
        static string discord_url = "https://discord.com/invite/jTac4sQ";
        static string webpage_url = "https://pwrgames.eu/";

        float downloads_calc = 0;
        float downloads_calc_progress = 0;
        float downloads_calc_current = 0;

        bool can_start = false;
        bool can_download = true;

        WebClient webClient;

        public Form1()
        {
            InitializeComponent();
            //bg
            this.FormBorderStyle = FormBorderStyle.None;

            //downloads filelist from web
            Process[] processes = Process.GetProcessesByName(System.IO.Path.GetFileNameWithoutExtension(System.Reflection.Assembly.GetEntryAssembly().Location));
            if (processes.Length > 1)
            {
                MessageBox.Show("The launcher already running! Close it, before running a new launcher!");
                Application.Exit();
                Environment.Exit(0);
            }

            Process[] processes2 = Process.GetProcessesByName("cabalmain");
            if (processes2.Length > 0)
            {

                MessageBox.Show("The cabalmain.exe already running! Close it, before running your launcher!");
                Application.Exit();
                Environment.Exit(0);
            }

            start_update();
        }

        public void calc_downloads()
        {
            downloads_calc = 0;
            downloads_calc_progress = 0;

            for (int i = 0; i < downloads_info.Rows.Count; i++)
            {
                FileInfo f = new FileInfo(downloads_info.Rows[i]["path"].ToString());

                if (!File.Exists(f + ""))
                {
                    int Number = int.Parse(downloads_info.Rows[i]["size"].ToString());
                    downloads_calc += Number;
                    continue;
                }

                if (downloads_info.Rows[i]["hash"].ToString() != GetMD5Checksum(f + ""))
                {
                    int Number = int.Parse(downloads_info.Rows[i]["size"].ToString());
                    downloads_calc += Number;

                }

            }

            draw_downloads_calc();
        }

        public void start_update()
        {
            pictureBox2.BackgroundImage = Properties.Resources.start2;
            can_start = false;

            if (RemoteFileExists(file_url))
            {
                using (webClient = new WebClient())
                {

                    webClient.DownloadFileCompleted += new AsyncCompletedEventHandler(Completed);
                    webClient.DownloadProgressChanged += new DownloadProgressChangedEventHandler(ProgressChanged);
                    webClient.DownloadFileAsync(new Uri(file_url), "files.xml", true);
                    label1.Text = "Checking launcher!";

                }
            }
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {
            File.Delete("files.xml");

            Application.Exit();
            Environment.Exit(0);
        }

        private void creatTable()
        {
            downloads_info = new DataTable("downloads_info");

            DataColumn column = new DataColumn();

            column = new DataColumn();
            column.DataType = System.Type.GetType("System.String");
            column.ColumnName = "path";
            column.ReadOnly = false;
            downloads_info.Columns.Add(column);

            column = new DataColumn();
            column.DataType = System.Type.GetType("System.String");
            column.ColumnName = "hash";
            column.ReadOnly = false;
            downloads_info.Columns.Add(column);

            column = new DataColumn();
            column.DataType = System.Type.GetType("System.Int32");
            column.ColumnName = "size";
            column.ReadOnly = false;
            downloads_info.Columns.Add(column);

            var size = reader.GetAttribute("size"); // get the data value
            if (size == null)
            {
                // if no value, then replace with the SQL required null value
                size = DBNull.Value;
            }
        }


        public void ReadXML()
        {
           
            if (!File.Exists("files.xml")) { return; } //if not exist return
            creatTable();

            using (XmlReader reader = XmlReader.Create("files.xml"))
            {
                reader.ReadToFollowing("critical");
                DataRow Row;

                do
                {
                    Row = downloads_info.NewRow();

                    reader.MoveToFirstAttribute();
                    Row["path"] = reader.GetAttribute("path");
                    Row["hash"] = reader.GetAttribute("hash");
                    Row["size"] = size;

                    downloads_info.Rows.Add(Row);
                } while (reader.ReadToFollowing("critical"));
            }


            calc_downloads();
        }

        private void Completed(object sender, AsyncCompletedEventArgs e)
        {
            

            if (!can_start)
            {
                ReadXML();

                get_and_check();
            }
        }

        public void get_and_check()
        {
            can_download = true;

            if (!File.Exists("files.xml")) return;

            for (int i = 0; i < downloads_info.Rows.Count; i++)
            {
                int Number = int.Parse(downloads_info.Rows[i]["size"].ToString());
                downloads_calc_current = Number;

                fileToBeZipped = new FileInfo(downloads_info.Rows[i]["path"].ToString());
                zipFileName = new FileInfo(string.Concat(fileToBeZipped, ".bz2"));
                
                if (!File.Exists( fileToBeZipped + "" )) 
                {
                    downlod_Zip();
                    break;
                    
                }

                else if (downloads_info.Rows[i]["hash"].ToString() != GetMD5Checksum(fileToBeZipped + ""))
                {
                    File.SetAttributes(fileToBeZipped + "", FileAttributes.Normal);

                    if (File.Exists("RED Launcher.exe") && downloads_info.Rows[i]["path"].ToString().Contains("Launcher"))
                    {
                        continue;
                    }

                    downlod_Zip();
                    break;
                }

                if (i == downloads_info.Rows.Count - 1)
                {
                    Start_game();
                }
            }
        }

        public void Start_game()
        {
            if (can_download) label1.Text = "Ready to start!";
            else
            {

                label1.Text = "Ready to start! But not up to date!";
                progressBar.Value = 0;

            }
            timer.Stop();

            pictureBox2.BackgroundImage = Properties.Resources.start_ready;

            can_start = true;
        }

        public void downlod_Zip()
        {
            string path = fileToBeZipped + ".bz2";

            if (RemoteFileExists(main_url + path) == false) return;
         
            label1.Text = "downloading -> " + fileToBeZipped.Name + "";

            if (!Directory.Exists(fileToBeZipped.DirectoryName))
            {
                DirectoryInfo di = Directory.CreateDirectory(fileToBeZipped.DirectoryName);
            }

            using (webClient = new WebClient())
            {
                webClient.DownloadFileCompleted += new AsyncCompletedEventHandler(Completed_files);
                webClient.DownloadProgressChanged += new DownloadProgressChangedEventHandler(ProgressChanged);
                webClient.DownloadFileAsync(new Uri(main_url + path), path, true);
            }
        }

        private void Completed_files(object sender, AsyncCompletedEventArgs e)
        {        
            if (!can_start)
            {
                using (FileStream fileToDecompressAsStream = zipFileName.OpenRead())
                {
                    string decompressedFileName = fileToBeZipped.FullName;
                    using (FileStream decompressedStream = File.Create(decompressedFileName))
                    {
                        try
                        {
                            BZip2.Decompress(fileToDecompressAsStream, decompressedStream, true);
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine(ex.Message);
                        }
                    }
                }
                File.Delete(zipFileName.FullName);

                downloads_calc_progress += downloads_calc_current;
                get_and_check();

                draw_downloads_calc();
            }
        }

        private void draw_downloads_calc()
        {
            label2.Text = ((downloads_calc_progress) / 1024 / 1024).ToString("0.00") + " MB / " + (downloads_calc / 1024 / 1024).ToString("0.00") + " MB";
        }

        private void ProgressChanged(object sender, DownloadProgressChangedEventArgs e)
        {
            progressBar.Value = e.ProgressPercentage;

            if(can_download)
            {
                timer.Stop();
                timer.Start();
            }
                
        }

        public static string GetMD5Checksum(string filename)//md5 check
        {
            using (var md5 = System.Security.Cryptography.MD5.Create())
            {
                using (var stream = System.IO.File.OpenRead(filename))
                {
                    var hash = md5.ComputeHash(stream);
                    return BitConverter.ToString(hash).Replace("-", "");
                }
            }
        }

        private bool RemoteFileExists(string url)
        {
            if (CheckForInternetConnection() )
            {
                WebRequest request = WebRequest.Create(new Uri(url));
                request.Method = "HEAD";

                using (WebResponse response = request.GetResponse())
                {
                    if (response.ContentLength == -1)
                    {
                        response.Close();
                        MessageBox.Show("File can't be found on this URL: " + url);
                        return false;
                    }
                    else
                    {
                        response.Close();
                        return true;
                    }
                }
            }
            else
            {
                MessageBox.Show("File can't be found on this URL: " + url);
                return false;
            }
        }

        public static bool CheckForInternetConnection()
        {
            try
            {
                using (var client = new WebClient())
                using (client.OpenRead("http://google.com/generate_204"))
                    return true;
            }
            catch
            {
                return false;
            }
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            can_download = false;
            webClient.Dispose();
            timer.Stop();

            MessageBox.Show("The client lost connection, or is unable to establish connection to a server for whatever reason! Check your internet connection! Maybe your client won't be up to date! What are can cause issues during gameplay!");
            MessageBox.Show("Try to RESTART your launcher!");

            Start_game();
        }

        private void pictureBox2_Click(object sender, EventArgs e)
        {
            if (can_start)
            {
                if (!File.Exists("cabalmain.exe"))
                {
                    can_start = false;
                    MessageBox.Show("The cabalmain.exe is not founded!");

                    start_update();

                    return;
                }


                string cPath = "";
                string cParams = "reddy";

                string filename = Path.Combine(cPath, "cabalmain.exe");
                var proc = Process.Start(filename, cParams);

                if (File.Exists("files.xml")) File.Delete("files.xml");

                Application.Exit();
                Environment.Exit(0);
            }
        }
        //webpage
        private void pictureBox4_Click(object sender, EventArgs e)
        {
            Process.Start(webpage_url);
        }

        private void pictureBox4_MouseHover(object sender, EventArgs e)
        {
            pictureBox4.BackgroundImage = Properties.Resources.web2;
        }

        private void pictureBox4_MouseLeave(object sender, EventArgs e)
        {
            pictureBox4.BackgroundImage = Properties.Resources.web;
        }
        //facebook
        private void pictureBox3_Click(object sender, EventArgs e)
        {
            Process.Start(facebook_url);
        }

        private void pictureBox3_MouseHover(object sender, EventArgs e)
        {
            pictureBox3.BackgroundImage = Properties.Resources.face2;
        }

        private void pictureBox3_MouseLeave(object sender, EventArgs e)
        {
            pictureBox3.BackgroundImage = Properties.Resources.face;
        }
        //discord
        private void pictureBox6_Click(object sender, EventArgs e)
        {
            Process.Start(discord_url);
        }

        private void pictureBox6_MouseHover(object sender, EventArgs e)
        {
            pictureBox6.BackgroundImage = Properties.Resources.dis2;
        }

        private void pictureBox6_MouseLeave(object sender, EventArgs e)
        {
            pictureBox6.BackgroundImage = Properties.Resources.dis;
        }
        //full check
        private void pictureBox5_Click(object sender, EventArgs e)
        {
            if(can_start) start_update();
        }

        private void pictureBox5_MouseHover(object sender, EventArgs e)
        {
            if(can_start) pictureBox5.BackgroundImage = Properties.Resources.full2;
        }

        private void pictureBox5_MouseLeave(object sender, EventArgs e)
        {
            pictureBox5.BackgroundImage = Properties.Resources.full;
        }

        private void notifyIcon1_DoubleClick(object sender, EventArgs e)
        {
            this.WindowState = FormWindowState.Normal;
        }

        private void pictureBox7_Click(object sender, EventArgs e)
        {
            this.WindowState = FormWindowState.Minimized;
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }
    }
}
