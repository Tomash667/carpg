using System;

namespace Blob
{
    public class BlobWorker
    {
        public string Work(string ver, string prevVer, string path, uint crc, bool recreate)
        {
            try
            {
                Config config = new Config();
                string url = BlobHandler.Upload(path, config.StorageConnectionString, recreate);
                ApiHandler.AddUpdate(ver, prevVer, url, config.ApiUrl, config.Token, crc);
                return null;
            }
            catch (Exception ex)
            {
                return ex.ToString();
            }
        }
    }
}
