namespace EthereumClasses
{
    public class Erc20Call : ContractCall
    {
        public Erc20Function EnumFunction => (Erc20Function) Function;
    }
}
