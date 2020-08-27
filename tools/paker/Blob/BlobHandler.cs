using Microsoft.Azure.Storage;
using Microsoft.Azure.Storage.Blob;
using System;
using System.IO;

namespace Blob
{
    class BlobHandler
    {
        public static string Upload(string path, string connectionString)
        {
            if (!File.Exists(path))
                throw new Exception($"Missing file '{path}'.");
            CloudStorageAccount account = CloudStorageAccount.Parse(connectionString);
            CloudBlobClient blobClient = account.CreateCloudBlobClient();
            CloudBlobContainer container = blobClient.GetContainerReference("updates");
            CloudBlockBlob blockBlob = container.GetBlockBlobReference(Path.GetFileName(path));
            blockBlob.UploadFromFile(path);
            return blockBlob.Uri.AbsoluteUri;
        }
    }
}
