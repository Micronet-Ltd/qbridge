using System;

using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace qbrdge_driver_classlib
{
    public struct LogLev
    {
        LogLev(bool showLog_, string name_) { showLog = showLog_; name = name_; }
        bool showLog;
        public bool ShowLog { get { return showLog; } }
        string name;
        public string Name { get { return name; } }
        public override string ToString() { return Name; }

        public static readonly LogLev Debug = new LogLev(true, "Debug");
        public static readonly LogLev Status = new LogLev(true, "Status");
        public static readonly LogLev Error = new LogLev(true, "Error");

    }

    public class Log
    {

        [DllImport("coredll.dll", EntryPoint = "OutputDebugString")]
        static extern void OutputDebugStringCE(string lpOutputString);
        static Action<string> OutputDebugString;
        static Log()
        {
            if (Environment.OSVersion.Platform == PlatformID.WinCE)
            {
                OutputDebugString = OutputDebugStringCE;
            }
            else
            {
                OutputDebugString = DebugWrite;
            }
        }

        static void DebugWrite(string msg)
        {
            Debug.Write(msg);
        }

        public static void Write(LogLev lev, string msg)
        {
            if (lev.ShowLog)
            {
                var now = DateTime.Now;
                string logMsg = string.Format("{0}  (C4Logic-{1}): {2}\n", now.ToString("hh:mm:ss dd-MMM-yy"), lev, msg);
                OutputDebugString(logMsg);

                AppendToFile(logMsg);
            }
        }

        [Conditional("DEBUG")]
        public static void WriteDebug(LogLev lev, string msg)
        {
            Write(lev, msg);
        }

        private static readonly int MaxLogLength = 100000;
        private static void AppendToFile(string msg)
        {
            TruncateFileIfNecessary();

            try
            {
                using (var sw = File.AppendText(LogFile))
                {
                    sw.Write(msg);
                }
            }
            catch { }
        }

        private static void TruncateFileIfNecessary()
        {
            FileInfo inf = new FileInfo(LogFile);
            if (inf.Exists && (inf.Length > MaxLogLength))
            {
                byte[] buff = null;
                using (FileStream fs = new FileStream(LogFile, FileMode.Open, FileAccess.Read))
                {
                    BinaryReader br = new BinaryReader(fs);
                    long numBytes = inf.Length;
                    buff = br.ReadBytes((int)numBytes);
                }

                using (FileStream fs = new FileStream(LogFile, FileMode.Create, FileAccess.Write))
                {
                    fs.Write(buff, buff.Length / 2, buff.Length - buff.Length / 2);
                }
            }
        }
        private static string appPath = null;
        public static string AppPath
        {
            get
            {
                if (appPath == null)
                {
                    appPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().GetName().CodeBase);
                    if (appPath.StartsWith("file:\\"))
                    {
                        appPath = appPath.Substring(6);
                    }
                }
                return appPath;
            }
        }
        private static string LogFile { get { return AppPath + "\\QBridgeExeLog.txt"; } }

    }
}
