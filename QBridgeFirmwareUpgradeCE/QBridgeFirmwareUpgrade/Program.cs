using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace QBridgeFirmwareUpgrade
{

    static class Program
    {
        public static string PortName;
        public static string FilePath;
        public static bool ConsoleMode;
        public static bool AutoRunGUI;

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [MTAThread]
        static void Main(String[] args)
        {
            UpdateFWForm updateFWForm = new UpdateFWForm();

            if (args.Length > 0)
            {
                ProcessArguments(args);
                if(ConsoleMode || AutoRunGUI)
                    Application.Run(updateFWForm);

            }
            else
            Application.Run(updateFWForm);
        }

        static void ProcessArguments(String[] args)
        {
            /*******************************/
            /* Show help */
            /*****************************/
            string help = "Upgrade the QBridge firmware\n" +
                            "Usage: QBridgeUpCE [COMPORT] [FILEPATH] [/i]\n" +
                            "\tNo args     Displays the graphical user interface (GUI)\n" +
                            "\tCOMPORT     Specify the COM port number or name to open\n" +
                            "\tFILEPATH    Specify the  path and file name of the\n" +
                            "\t            QBridge firmware\n" +
                            "\t            (e.g. \\USBHardDisk\\v1_006.srec\n" +
                            "\t/i          Displays the graphical user interface (GUI)\n" +
                            "\t            and automates upgrade using specified\n" +
                            "\t            parameters. Omit this to upgrade in console";


            // Show help
            if (args[0].Trim().StartsWith("/?") ||
                args[0].Trim().StartsWith("/h") ||
                args[0].Trim().StartsWith("-?") ||
                args[0].Trim().StartsWith("-h") ||
                args.Length > 3 ||
                args.Length < 2)
            {
                Console.WriteLine(help);
    
                return;
            }

            /*******************************/
            /* Check if file path is valid*/
            /*****************************/

            bool valid_filepath = false;

            try
            {
                FilePath = System.IO.Path.GetFullPath(args[1]);
                //System.IO.FileStream fs = new System.IO.FileStream(FilePath, System.IO.FileMode.Open);

                valid_filepath = FilePath.Equals("") ? false : true;

            }
            catch (Exception) { }
            // Invalid file path, show help
            if (!valid_filepath)
            {
                // Show help and exit
                Console.WriteLine(help);
                return;
            }

            /*******************************/
            /* Check if COM port is valid */
            /*****************************/
            // Make sure number after "COM" is valid, if so then store this as port name
            if (args[0].Trim().StartsWith("COM")
            && TryParse(args[0].Trim().Substring(3)))
            {
                PortName = args[0].Trim();
            }
            // Verify number and add "COM"
            else if (TryParse(args[0].Trim()))
            {
                PortName = "COM" + args[0].Trim();
            }
            else // Invalid COM port parameter
            {
                // Show help and exit
                Console.WriteLine(help);
                return;
            }

            /*******************************/
            /* GUI parameter              */
            /*****************************/
            if (args.Length >= 3)
            {
                // disable console mode, enable gui autorun
                if (args[2].Trim().Equals("/i"))
                {
                    ConsoleMode = false;
                    AutoRunGUI = true;
                }
            }
            // enable console mode, no gui autorun
            else
            {
                ConsoleMode = true;
                AutoRunGUI = false;
            }
        }

        static bool TryParseImpl(string s, int start, ref int value)
        {
            if (start == s.Length)
                return false;
            unchecked
            {
                int i = start;
                do
                {
                    int newvalue = value * 10 + '0' - s[i++];
                    if (value != newvalue / 10) { value = 0; return false; } // detect non-digits and overflow all at once
                    value = newvalue;
                } while (i < s.Length);
                if (start == 0)
                {
                    if (value == int.MinValue) { value = 0; return false; }
                    value = -value;
                }
            }
            return true;
        }

        static bool TryParse(string s)
        {
            int value = 0;
            if (s == null) return false;
            s = s.Trim();
            if (s.Length == 0) return false;
            return TryParseImpl(s, (s[0] == '-') ? 1 : 0, ref value);
        }
    }
}