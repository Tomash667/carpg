using Microsoft.Azure.Storage;
using Microsoft.Azure.Storage.Blob;
using System;
using System.IO;

namespace Blob
{
    class BlobHandler
    {
        public static string Upload(string path, string connectionString, bool recreate)
        {
            if (!File.Exists(path))
                throw new Exception($"Missing file '{path}'.");
            CloudStorageAccount account = CloudStorageAccount.Parse(connectionString);
            CloudBlobClient blobClient = account.CreateCloudBlobClient();
            CloudBlobContainer container = blobClient.GetContainerReference("updates");
            CloudBlockBlob blockBlob = container.GetBlockBlobReference(Path.GetFileName(path));
            if(!blockBlob.Exists() || recreate)
                blockBlob.UploadFromFile(path);
            string url = blockBlob.Uri.AbsoluteUri;
            url = url.Replace("https", "http");
            return url;
        }
    }
}
