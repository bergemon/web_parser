#include "client.hpp"

void Network::Client::resolveHandler(const boost::system::error_code& ec, tcp::resolver::iterator ep_iterator) {
	if (!ec) {
		m_socket.set_verify_mode(boost::asio::ssl::verify_peer);
		m_socket.set_verify_callback([&](bool preverified, boost::asio::ssl::verify_context& ctx) {
			char subject_name[256];
			X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
			X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		#ifdef DEBUG
			std::cout << "Verifying " << subject_name << "\n";
		#endif

			return true;
		});
		m_socket.lowest_layer().async_connect(ep_iterator->endpoint(),
			boost::bind(&Client::connectionHandler, this, asio::placeholders::error));
	}
	else {
		std::cout << "[Resolve] Error: " << ec.what() << '\n';
	}
}
void Network::Client::connectionHandler(const boost::system::error_code& ec) {
	if (!ec) {
	#ifdef DEBUG
		std::cout << "[Connection] Connected to " << m_url.host() << '\n';
	#endif

		m_socket.async_handshake(asio::ssl::stream_base::handshake_type::client,
			boost::bind(&Client::handshakeHandler, this, asio::placeholders::error));
	}
	else {
		std::cout << "[Connection] Error: " << ec.what() << '\n';
	}
}
void Network::Client::handshakeHandler(const boost::system::error_code& ec) {
	if (!ec) {
	#ifdef DEBUG
		std::cout << "[Handshake] " << "OK\n";
	#endif

		std::ostream os(&m_request);

		os << "GET " << m_url.path() << " HTTP/1.0\r\n";
		os << "Host: " << m_url.host() << "\r\n";
		os << "Accept: */*\r\n";
		os << "Connection: close\r\n\r\n";

		asio::async_write(m_socket, m_request, boost::bind(&Client::requestHandler, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
	}
	else {
		std::cout << "[Handshake] Error: " << ec.what() << '\n';
	}
}
void Network::Client::requestHandler(const boost::system::error_code& ec, size_t bytes) {
	if (!ec) {
	#ifdef DEBUG
		std::cout << "[Request] Sent: " << bytes << " bytes to " << m_url.host() << '\n';
	#endif

		asio::async_read_until(m_socket, m_response, "\r\n",
			boost::bind(&Client::readStatusHandler, this, asio::placeholders::error));
	}
	else {
		std::cout << "[Request] Error: " << ec.what() << '\n';
	}
}
void Network::Client::readStatusHandler(const boost::system::error_code& ec) {
	if (!ec) {
		std::istream is(&m_response);

	#ifdef DEBUG
		std::string header;
		is >> header;
		std::cout << "Protocol: " << header << '\n';
		std::string status_code;
		is >> status_code;
		std::cout << "Status: " << status_code << '\n';
	#endif
		std::string status_message;
		std::getline(is, status_message);

		asio::async_read_until(m_socket, m_response, "\r\n\r\n",
			boost::bind(&Client::readHeaderLinesHandler, this, asio::placeholders::error));
	}
	else if (ec && ec != asio::error::eof) {
		std::cout << "[Read status] Error: " << ec.what() << '\n';
	}
}
void Network::Client::readHeaderLinesHandler(const boost::system::error_code& ec) {
	if (!ec) {
		std::istream is(&m_response);

		std::string header_line;

		while (getline(is, header_line) && header_line != "\r") {
		#ifdef DEBUG
			std::cout << header_line;
		#endif
		}

		asio::async_read(m_socket, m_response, asio::transfer_at_least(1),
			boost::bind(&Client::readResponseBodyHandler, this, asio::placeholders::error));
	}
	else if (ec && ec != asio::error::eof) {
		std::cout << "[Read header lines] Error: " << ec.what() << '\n';
	}
}
void Network::Client::readResponseBodyHandler(const boost::system::error_code& ec) {
	if (!ec) {
		asio::async_read(m_socket, m_response, asio::transfer_at_least(1),
			boost::bind(&Client::readResponseBodyHandler, this, asio::placeholders::error));
	}
	else if (ec && ec != asio::error::eof) {
		std::cout << "[Read response body] Error: " << ec.what() << '\n';
	}
}