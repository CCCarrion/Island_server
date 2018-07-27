using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net.Sockets;
using System.Net;

namespace ServerCode
{
    class Program
    {
        static void Main(string[] args)
        {

            Socket listenfd = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);//创建一个套接字，第一个参数是地址族，第二个是套接字类型，游戏一般是stream，第三个是使用的协议
            IPAddress ipAdr = IPAddress.Parse("127.0.0.1");//ip地址
            IPEndPoint ipEp = new IPEndPoint(ipAdr, 1234);//ip地址和端口号
            listenfd.Bind(ipEp);//给套接字绑定
            listenfd.Listen(0);//开启监听，等待连接
            Console.WriteLine("服务器启动成功");
            while (true)
            {
                Socket connfd = listenfd.Accept();//接受客户端的连接，本例中所有的socket方法都是阻塞方法，accept返回新客户端的socket
                Console.WriteLine("服务器accept");
                byte[] readBuff = new byte[1024];
                int count = connfd.Receive(readBuff);//接收到的信息存在readBuff中
                string str = System.Text.Encoding.UTF8.GetString(readBuff, 0, count);//将字节转化为字符串
                Console.WriteLine("服务器接收" + str);
                byte[] bytes = System.Text.Encoding.Default.GetBytes("server " + str);
                connfd.Send(bytes);//给客户端发消息
            }

        }
    }
}
