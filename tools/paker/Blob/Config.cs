using Newtonsoft.Json;
using System.IO;

namespace Blob
{
    class Config
    {
        public string StorageConnectionString { get; set; }
        public string ApiUrl { get; set; }
        public string Token { get; set; }

        public Config()
        {
            string json = File.ReadAllText("blob.json");
            JsonConvert.PopulateObject(json, this);
        }
    }
}
