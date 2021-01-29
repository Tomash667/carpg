using Newtonsoft.Json;
using RestSharp;
using System;

namespace Blob
{
    class ApiHandler
    {
        class RequestBody
        {
            public string From { get; set; }
            public string To { get; set; }
            public string Path { get; set; }
            public uint Crc { get; set; }
        }

        class ResponseBody
        {
            public bool Ok { get; set; }
            public string Error { get; set; }
        }

        public static void AddUpdate(string ver, string prevVer, string path, string apiUrl, string token, uint crc)
        {
            RestClient client = new RestClient(apiUrl);
            client.AddDefaultHeader("Authorization", $"Bearer {token}");
            RestRequest request = new RestRequest("api/update", Method.POST);
            request.AddJsonBody(new RequestBody
            {
                From = prevVer,
                To = ver,
                Path = path,
                Crc = crc
            });
            IRestResponse response = client.Execute(request);
            if (response.StatusCode != System.Net.HttpStatusCode.OK)
                throw new Exception($"Invalid api response {response.StatusCode}: {response.Content}");
            ResponseBody resp = JsonConvert.DeserializeObject<ResponseBody>(response.Content);
            if (!resp.Ok)
                throw new Exception($"Api error response: {resp.Error}");
        }
    }
}
