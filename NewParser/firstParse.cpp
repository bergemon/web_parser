#include "firstParse.hpp"


void Network::FirstParse::readHeaderLinesHandler(const boost::system::error_code& ec) {
	std::ofstream file("tempFile.txt", std::ios::out);

	if (!ec) {
		std::istream is(&m_response);

		std::string header_line;

		while (getline(is, header_line) && header_line != "\r") {
		#ifdef DEBUG
			std::cout << header_line;
		#endif
	}

		asio::async_read(m_socket, m_response, asio::transfer_at_least(1),
			boost::bind(&FirstParse::readResponseBodyHandler, this, asio::placeholders::error));
}
	else if (ec && ec != asio::error::eof) {
		std::cout << "[Read header lines] Error: " << ec.what() << '\n';
	}
}
void Network::FirstParse::readResponseBodyHandler(const boost::system::error_code& ec) {
	std::ofstream file("tempFile.txt", std::ios::out | std::ios::app);

	if (!ec) {
		file << &m_response;

		asio::async_read(m_socket, m_response, asio::transfer_at_least(1),
			boost::bind(&FirstParse::readResponseBodyHandler, this, asio::placeholders::error));
	}
	else if (ec == asio::error::eof) {
		if (file.is_open())
			file.close();
	}
	else if (ec && ec != asio::error::eof) {
		std::cout << "[Read response body] Error: " << ec.what() << '\n';
	}
}