using System.Text;
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
            var text = new StringBuilder();
            text.Append(request.X);
            text.Append(' ');

            switch (request.Op)
            {
                case Calc.Managed.Operation.Add:
                    response.Z = request.X + request.Y;
                    text.Append('+');
                    break;

                case Calc.Managed.Operation.Subtract:
                    response.Z = request.X - request.Y;
                    text.Append('-');
                    break;

                case Calc.Managed.Operation.Multiply:
                    response.Z = request.X * request.Y;
                    text.Append('*');
                    break;

                case Calc.Managed.Operation.Divide:
                    response.Z = request.X / request.Y;
                    text.Append('/');
                    break;
            }

            text.Append(' ');
            text.Append(request.Y);
            text.Append(" = ");
            response.Text = text.ToString();
            
            return response;
        }
    }
}
