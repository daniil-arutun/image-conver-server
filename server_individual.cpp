#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <fstream>
#include <vector>
#include <opencv2/opencv.hpp>

using boost::asio::ip::tcp;


const size_t CHUNK_SIZE = 1024;//опять создаем константый чанк
/*
void save_file_from_vector(const std::vector<char>& data, const std::string& file_path) {//эта функция просто создает офстрим который сохраняет файл
    std::ofstream file(file_path, std::ios::binary);//создаем
    file.write(data.data(), data.size());//и эта функция записывает нужные данные
}
*/
class con : public std::enable_shared_from_this<con>
{
public:
    typedef std::shared_ptr<con> pointer;

    static pointer create(boost::asio::io_context& io_context)
    {
        return pointer(new con(io_context));
    }


    tcp::socket& socket()
    {
        return socket_;
    }


    void read()//начинаем читать 
    {
        auto self = shared_from_this();//по стандарту создаем самообъект
        boost::asio::async_read(socket_, boost::asio::buffer(&file_size_, sizeof(file_size_)),//тут важный момент 
            //здесь нужен именоо асинк_рид а не асинк_рид_сом
            //нужно прочитать весь размер разом
            [this, self](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    file_data_.resize(file_size_);//меняем размер на полученный от клиента
                    read_file_data();//начинаем считывать
                }
                else {
                    std::cerr << "Error reading file size: " << ec.message() << "\n";
                }
            });

    }

    void read_file_data() {
        auto self = shared_from_this();//по стандарту
        size_t remaining_bytes = file_size_ - received_bytes_;//если первый раз то рецив равен 0 но эта функция вызывается несколько раз
//так что потом будетн не 0
        size_t chunk_size = std::min(CHUNK_SIZE, remaining_bytes);//ну как обычно минимальный чанк

        socket_.async_read_some(boost::asio::buffer(file_data_.data() + received_bytes_, chunk_size),//вот тут уже читаем через асинк рид сом так как отправляют частями
            //и как у клиента в буффере по индексу идем и записывается с разных мест
            [this, self, chunk_size](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    received_bytes_ += bytes_transferred;//тут уже меняем полученные байты
                    if (received_bytes_ < file_size_) {//и проверяем если меньше то вызываем вновь
                        read_file_data();
                    }
                    else {//но как будет равен то записываем сюда

                  //      save_file_from_vector(file_data_, "D:/img/i/received_image.jpg");
                        get_for();
                        std::cout << "File received and saved.\n";
                    }
                }
                else {
                    std::cerr << "Error reading file data: " << ec.message() << "\n";
                }
            });
    }

    void get_for()
    {
        auto self = shared_from_this();

      
       socket_.async_read_some(boost::asio::buffer(mas), [self,this](boost::system::error_code ec,std::size_t t) 
            {
               if (!ec)
               {
                   peredel();
               }
               else
               {
                   std::cout << ec.message() << "\n";
               }
            });
        
    }
    void peredel()
    {
        std::cout << mas << "\n";
        cv::Mat inputImage = cv::imdecode(file_data_, cv::IMREAD_UNCHANGED);
        if (inputImage.empty())
        {
            std::cout << "Empty\n";
        }

        inputImage.convertTo(inputImage,CV_16UC1 ,256);

        if (!cv::imencode(mas,inputImage,file_data_2_))
        {
            std::cout << "Error\n";
        }
        if (file_data_2_.empty())
        {
            std::cout << "EEror\n";
        }
        write();
    }

    void write()
    {
        auto self = shared_from_this();
        tot_siz= file_data_2_.size();//берем размер
        fd.insert(fd.begin(),file_data_2_.begin(),file_data_2_.end());

        boost::asio::async_write(socket_, boost::asio::buffer(&tot_siz,sizeof(tot_siz)), [this, self](boost::system::error_code ec, std::size_t bytes_sent) {
            if (!ec)
            {
                write_file_data();
            }
            else
            {
                std::cout << "error:" << ec.message() << "\n";
            }
            });

    }

    void write_file_data()
    {
        auto p = shared_from_this();
      //  std::vector<char> fd(file_data_2_.begin(), file_data_2_.end());
        if (sen < tot_siz)
        {
            size_t chun= std::min(CHUNK_SIZE, tot_siz - sen);

            socket_.async_write_some(boost::asio::buffer(fd.data() + sen, chun), [this,p](boost::system::error_code ec, std::size_t bytes_sent) 
                {
                    if (!ec)
                    {
                        sen += bytes_sent;
                        write_file_data();
                    }
                    else
                    {
                        std::cout << "error:" << ec.message() << "\n";
                    }
                });

        }
        else
        {
            std::cout << "finished\n";
        }
    }

private:
    con(boost::asio::io_context& c) : socket_(c)
    {}

    tcp::socket socket_;
    std::vector<char> file_data_;//дата которую получаем
    std::vector<unsigned char> file_data_2_;
    std::vector<char> fd;
    size_t file_size_ = 0;//размер файла
    size_t received_bytes_ = 0;//ну полученный байты
    size_t sen = 0;
    size_t tot_siz = 0;
    std::string format;
    char mas[255];
};


class ser : public std::enable_shared_from_this<ser>
{
public:
    ser(boost::asio::io_context& i) : co(i), ac(co, tcp::endpoint(tcp::v4(), 8383))

    {}

    void start()
    {
        auto self = shared_from_this();
        con::pointer p = con::create(co);

        ac.async_accept(p->socket(), [this, self, p](const boost::system::error_code& e)
            {
                if (!e)
                {
                    p->read();
                }
                else
                {
                    std::cout << "Accept error: " << e.message() << "\n";
                }

                start();
            });
    }


private:
    boost::asio::io_context& co;
    tcp::acceptor ac;

};

int main()
{
    try
    {
        boost::asio::io_context io;

        auto server = std::make_shared<ser>(io);
        server->start();

        io.run();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << "\n";
    }
    return 0;
}
