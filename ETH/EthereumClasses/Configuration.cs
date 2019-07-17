using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using EthereumClasses;
using Newtonsoft.Json;

namespace EthereumClasses
{
    public class Configuration
    {
        [JsonProperty("hostname")]
        private string _hostname = "localhost";
        [JsonProperty("rpc_port")]
        private int? _rpcPort;
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
                ret.Fixup();
                return ret;
            }
        }

        private void Fixup()
        {
            if (_binBuffers)
            {
                if (_dbPath == null)
                    throw new Exception("db_path must be defined in the configuration!");
                if (_inputsPath == null)
                    throw new Exception("inputs_path must be defined in the configuration!");
                if (_logsPath == null)
                    throw new Exception("logs_path must be defined in the configuration!");
                if (_restPort == null)
                    throw new Exception("rest_port must be defined in the configuration!");
            }

            if (_testnet.HasValue)
                _network = (int)(!_testnet.Value ? NetworkType.Ethereum : NetworkType.Ropsten);

            if (!_network.HasValue)
                throw new Exception("network must be defined in the configuration!");

            if (!_rpcPort.HasValue)
                _rpcPort = ((NetworkType)_network.Value).NetworkIsEthereum() ? 8545 : 4444;
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
        public int RpcPort => _rpcPort.Value;
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
        public NetworkType Network => _network.HasValue ? (NetworkType) _network.Value : NetworkType.Ethereum;
    }
}
