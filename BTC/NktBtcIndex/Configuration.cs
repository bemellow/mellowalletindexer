using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace NktBtcIndex
{
    class Configuration
    {
        [JsonProperty("hostname")]
        private string _hostname = "localhost";
        [JsonProperty("rpc_port")]
        private int _rpcPort = 8332;
        [JsonProperty("testnet")]
        private bool _testnet = false;
        [JsonProperty("rpc_user")]
        private string _rpcUser;
        [JsonProperty("rpc_password")]
        private string _rpcPassword;
        [JsonProperty("db_path")]
        private string _dbPath;
        [JsonProperty("rest_port")]
        private int? _restPort;
        [JsonIgnore]
        private static Configuration _config;

        private static Configuration Load()
        {
            using (var file = new StreamReader(@"BtcIndex.conf"))
            {
                var ret = JsonConvert.DeserializeObject<Configuration>(file.ReadToEnd());
                if (ret._rpcUser == null)
                    throw new Exception("rpc_user must be defined in the configuration!");
                if (ret._rpcPassword == null)
                    throw new Exception("rpc_password must be defined in the configuration!");
                if (ret._dbPath == null)
                    throw new Exception("db_path must be defined in the configuration!");
                if (ret._restPort == null)
                    throw new Exception("rest_port must be defined in the configuration!");
                return ret;
            }
        }

        public static Configuration Get()
        {
            if (_config == null)
                _config = Load();
            return _config;
        }

        [JsonIgnore]
        public string Url => $"http://{_hostname}:{_rpcPort}";
        [JsonIgnore]
        public int RpcPort => _rpcPort;
        [JsonIgnore]
        public string RpcUser => _rpcUser;
        [JsonIgnore]
        public string RpcPassword => _rpcPassword;
        [JsonIgnore]
        public string DbPath => _dbPath;
        [JsonIgnore]
        public bool Testnet => _testnet;
        [JsonIgnore]
        public int RestPort => _restPort.Value;
    }
}
