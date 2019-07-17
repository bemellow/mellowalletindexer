using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace Configuration
{
    public class Configuration
    {
        [JsonProperty("hostname")]
        private string _hostname = "localhost";
        [JsonProperty("rpc_port")]
        private int _rpcPort = 8545;
        [JsonProperty("db_path")]
        private string _dbPath;
        [JsonProperty("inputs_path")]
        private string _inputsPath;
        [JsonProperty("logs_path")]
        private string _logsPath;
        [JsonProperty("binary_buffers")]
        private bool _binBuffers = false;
        [JsonProperty("rest_port")]
        private int? _restPort;
        [JsonProperty("testnet")]
        private bool? _testnet;
        [JsonProperty("network")]
        private int? _network;
        [JsonIgnore]
        private static Configuration _config;

        private static Configuration Load()
        {
            using (var file = new StreamReader(@"EthIndex.conf"))
            {
                var ret = JsonConvert.DeserializeObject<Configuration>(file.ReadToEnd());
                if (ret._binBuffers)
                {
                    if (ret._dbPath == null)
                        throw new Exception("db_path must be defined in the configuration!");
                    if (ret._inputsPath == null)
                        throw new Exception("inputs_path must be defined in the configuration!");
                    if (ret._logsPath == null)
                        throw new Exception("logs_path must be defined in the configuration!");
                    if (ret._restPort == null)
                        throw new Exception("rest_port must be defined in the configuration!");
                }

                if (ret._testnet.HasValue)
                {
                    ret._network = ret._testnet.Value ? (int)
                }
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
        public string Url => $"http://{_hostname}:{_rpcPort}/";
        [JsonIgnore]
        public int RpcPort => _rpcPort;
        [JsonIgnore]
        public string DbPath => _dbPath;
        [JsonIgnore]
        public string InputsPath => _inputsPath;
        [JsonIgnore]
        public string LogsPath => _logsPath;
        [JsonIgnore]
        public bool RecordBinaryBuffers => _binBuffers;
        [JsonIgnore]
        public int RestPort => _restPort.Value;
        [JsonIgnore]
        public bool Testnet => _testnet;
    }
}
