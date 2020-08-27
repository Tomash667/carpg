using System;

namespace Blob
{
    public class BlobWorker
    {
        public string Work(string ver, string prevVer, string path)
        {
            try
            {
                Config config = new Config();
                string url = BlobHandler.Upload(path, config.StorageConnectionString);
                ApiHandler.AddUpdate(ver, prevVer, url, config.ApiUrl, config.Token);
                return null;
            }
            catch (Exception ex)
            {
                return ex.ToString();
            }
        }
    }
}
