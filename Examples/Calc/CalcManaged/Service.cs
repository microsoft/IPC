using System.Threading.Tasks;
using IPC.Managed;

namespace CalcManaged
{
    internal class Service
    {
        private readonly SharedMemory _memory;

        public Service(SharedMemory memory)
        {
            _memory = memory;
        }

        public async Task<Calc.Managed.Response> Invoke(Calc.Managed.Request request)
        {
            var response = new Calc.Managed.Response(_memory);
            response.Text = request.X + " ";

            switch (request.Op)
            {
                case Calc.Managed.Operation.Add:
                    response.Z = request.X + request.Y;
                    response.Text += '+';
                    break;

                case Calc.Managed.Operation.Subtract:
                    response.Z = request.X - request.Y;
                    response.Text += '-';
                    break;

                case Calc.Managed.Operation.Multiply:
                    response.Z = request.X * request.Y;
                    response.Text += '*';
                    break;

                case Calc.Managed.Operation.Divide:
                    response.Z = request.X / request.Y;
                    response.Text += '/';
                    break;
            }

            response.Text += " " + request.Y + " = ";
            
            return response;
        }
    }
}
