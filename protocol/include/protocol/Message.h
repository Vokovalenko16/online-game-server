#pragma once

#include "Types.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <stdexcept>
#include <new>
#include <algorithm>
#include <utility>
#include <array>
#include <boost/archive/detail/common_iarchive.hpp>
#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/serialization/array.hpp>

namespace protocol {

	struct MessageHeader
	{
		std::uint32_t length;
		std::uint32_t id;
		MessageType type;

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & length;
			ar & id;
			ar & type;
		}
	};

	union Message
	{
		MessageHeader header;
		std::uint8_t raw_data[MaxMessageLen];
	};

	class VariableMessage
	{
	private:
		MessageHeader* data_;
		size_t capacity_;

		explicit VariableMessage(size_t capacity)
			: data_((MessageHeader*)::operator new(capacity))
			, capacity_(capacity)
		{}

	public:
		VariableMessage()
			: data_(nullptr)
			, capacity_(0)
		{}

		~VariableMessage()
		{
			if (data_)
				::operator delete(data_);
		}

		VariableMessage(const Message& msg)
			: VariableMessage(msg.header.length)
		{
			std::memcpy(data_, msg.raw_data, msg.header.length);
		}

		VariableMessage(VariableMessage&& o)
			: data_(o.data_)
			, capacity_(o.capacity_)
		{
			o.data_ = nullptr;
			o.capacity_ = 0;
		}

		VariableMessage(const VariableMessage&) = delete;

		void swap(VariableMessage& o)
		{
			std::swap(data_, o.data_);
			std::swap(capacity_, o.capacity_);
		}

		VariableMessage& operator =(VariableMessage o)
		{
			swap(o);
			return *this;
		}

		void expand(size_t len)
		{
			if (capacity_ >= len)
				return;

			if (len > MaxMessageLen)
				throw std::invalid_argument("message length exceeded maximum.");

			size_t grow = capacity_ / 2;
			grow = (std::max)(grow, len - capacity_);
			grow = (std::max)(grow, (size_t)16);
			grow = (std::min)(grow, MaxMessageLen - capacity_);

			VariableMessage other{ capacity_ + grow };
			std::memcpy(other.data_, data_, capacity_);

			swap(other);
		}

		MessageHeader& get() const { return *data_; }
		Message& getMessage() const {
			return *(Message*)data_;
		}
	};


	// Message archives.
	class MessageWriter
		: public boost::archive::detail::common_oarchive<MessageWriter>
	{
	private:
		Message* msg_;

	public:
		MessageWriter(Message& msg)
		{
			msg_ = &msg;
			msg_->header.length = sizeof(MessageHeader);
		}

	private:
		std::uint8_t* verify_and_expand(size_t count)
		{
			if (msg_->header.length + count > MaxMessageLen)
				throw std::range_error("message length overflow.");

			std::uint8_t* res = msg_->raw_data + msg_->header.length;
			msg_->header.length += (std::uint32_t)count;

			return res;
		}

	public:
		void save_binary(const void* address, size_t count)
		{
			std::memcpy(verify_and_expand(count), address, count);
		}

	private:
		friend class boost::archive::save_access;
		template<typename T> void save(const T& t) { return save_binary(&t, sizeof(t)); }

		void save(const boost::serialization::collection_size_type& x)
		{
			std::uint32_t u = (std::uint32_t)(size_t)x;
			return save_binary(&u, sizeof(u));
		}
	};

	class VariableMessageWriter
		: public boost::archive::detail::common_oarchive<VariableMessageWriter>
	{
	private:
		VariableMessage* msg_;

	public:
		VariableMessageWriter(VariableMessage& msg)
		{
			msg_ = &msg;
			msg_->expand(sizeof(MessageHeader));
			msg_->get().length = sizeof(MessageHeader);
		}

	private:
		std::uint8_t* verify_and_expand(size_t count)
		{
			msg_->expand(msg_->get().length + count);

			auto& data = msg_->get();

			std::uint8_t* res = (std::uint8_t*)&data + data.length;
			data.length += (std::uint32_t)count;

			return res;
		}

	public:
		void save_binary(const void* address, size_t count)
		{
			std::memcpy(verify_and_expand(count), address, count);
		}

	private:
		friend class boost::archive::save_access;
		template<typename T> void save(const T& t) { return save_binary(&t, sizeof(t)); }
		
		void save(const boost::serialization::collection_size_type& x)
		{
			std::uint32_t u = (std::uint32_t)(size_t)x;
			return save_binary(&u, sizeof(u));
		}
	};

	class MessageReader
		: public boost::archive::detail::common_iarchive<MessageReader>
	{
	private:
		const Message* msg_;
		size_t pos_;

	public:
		MessageReader(const Message& msg)
		{
			msg_ = &msg;
			pos_ = sizeof(MessageHeader);
		}

	private:
		const std::uint8_t* verify_and_consume(size_t count)
		{
			if (pos_ + count > msg_->header.length)
				throw std::range_error("reached end of message.");

			const std::uint8_t* res = msg_->raw_data + pos_;
			pos_ += (std::uint32_t)count;

			return res;
		}

	public:
		void load_binary(void* address, size_t count)
		{
			std::memcpy(address, verify_and_consume(count), count);
		}

	private:
		friend class boost::archive::load_access;
		template<typename T> void load(T& t) { return load_binary(&t, sizeof(t)); }
		
		void load(boost::serialization::collection_size_type& x)
		{
			std::uint32_t u;
			load_binary(&u, sizeof(u));
			x = (size_t)u;
		}
	};



	template<> inline void MessageWriter::save(const std::uint8_t& t)
	{
		*verify_and_expand(1) = t;
	}

	template<> inline void MessageWriter::save(const std::uint16_t& t)
	{
		auto p = verify_and_expand(2);
		p[0] = (std::uint8_t)(t);
		p[1] = (std::uint8_t)(t >> 8);
	}

	template<> inline void MessageWriter::save(const std::uint32_t& t)
	{
		auto p = verify_and_expand(4);
		p[0] = (std::uint8_t)(t);
		p[1] = (std::uint8_t)(t >> 8);
		p[2] = (std::uint8_t)(t >> 16);
		p[3] = (std::uint8_t)(t >> 24);
	}

	template<> inline void MessageWriter::save(const std::uint64_t& t)
	{
		auto p = verify_and_expand(8);
		p[0] = (std::uint8_t)(t);
		p[1] = (std::uint8_t)(t >> 8);
		p[2] = (std::uint8_t)(t >> 16);
		p[3] = (std::uint8_t)(t >> 24);
		p[4] = (std::uint8_t)(t >> 32);
		p[5] = (std::uint8_t)(t >> 40);
		p[6] = (std::uint8_t)(t >> 48);
		p[7] = (std::uint8_t)(t >> 56);
	}

	template<> inline void MessageWriter::save(const std::int8_t& t)
	{
		return save((std::uint8_t)t);
	}

	template<> inline void MessageWriter::save(const std::int16_t& t)
	{
		return save((std::uint16_t)t);
	}

	template<> inline void MessageWriter::save(const std::int32_t& t)
	{
		return save((std::uint32_t)t);
	}

	template<> inline void MessageWriter::save(const std::int64_t& t)
	{
		return save((std::uint64_t)t);
	}



	template<> inline void VariableMessageWriter::save(const std::uint8_t& t)
	{
		*verify_and_expand(1) = t;
	}

	template<> inline void VariableMessageWriter::save(const std::uint16_t& t)
	{
		auto p = verify_and_expand(2);
		p[0] = (std::uint8_t)(t);
		p[1] = (std::uint8_t)(t >> 8);
	}

	template<> inline void VariableMessageWriter::save(const std::uint32_t& t)
	{
		auto p = verify_and_expand(4);
		p[0] = (std::uint8_t)(t);
		p[1] = (std::uint8_t)(t >> 8);
		p[2] = (std::uint8_t)(t >> 16);
		p[3] = (std::uint8_t)(t >> 24);
	}

	template<> inline void VariableMessageWriter::save(const std::uint64_t& t)
	{
		auto p = verify_and_expand(8);
		p[0] = (std::uint8_t)(t);
		p[1] = (std::uint8_t)(t >> 8);
		p[2] = (std::uint8_t)(t >> 16);
		p[3] = (std::uint8_t)(t >> 24);
		p[4] = (std::uint8_t)(t >> 32);
		p[5] = (std::uint8_t)(t >> 40);
		p[6] = (std::uint8_t)(t >> 48);
		p[7] = (std::uint8_t)(t >> 56);
	}

	template<> inline void VariableMessageWriter::save(const std::int8_t& t)
	{
		return save((std::uint8_t)t);
	}

	template<> inline void VariableMessageWriter::save(const std::int16_t& t)
	{
		return save((std::uint16_t)t);
	}

	template<> inline void VariableMessageWriter::save(const std::int32_t& t)
	{
		return save((std::uint32_t)t);
	}

	template<> inline void VariableMessageWriter::save(const std::int64_t& t)
	{
		return save((std::uint64_t)t);
	}



	template<> inline void MessageReader::load(std::uint8_t& t)
	{
		t = *verify_and_consume(1);
	}

	template<> inline void MessageReader::load(std::uint16_t& t)
	{
		auto p = verify_and_consume(2);
		t = (std::uint16_t)p[0]
			| ((std::uint16_t)p[1] << 8);
	}

	template<> inline void MessageReader::load(std::uint32_t& t)
	{
		auto p = verify_and_consume(4);
		t = (std::uint32_t)p[0]
			| ((std::uint32_t)p[1] << 8)
			| ((std::uint32_t)p[2] << 16)
			| ((std::uint32_t)p[3] << 24);
	}

	template<> inline void MessageReader::load(std::uint64_t& t)
	{
		auto p = verify_and_consume(8);
		t = (std::uint64_t)p[0]
			| ((std::uint64_t)p[1] << 8)
			| ((std::uint64_t)p[2] << 16)
			| ((std::uint64_t)p[3] << 24)
			| ((std::uint64_t)p[4] << 32)
			| ((std::uint64_t)p[5] << 40)
			| ((std::uint64_t)p[6] << 48)
			| ((std::uint64_t)p[7] << 56);
	}

	template<> inline void MessageReader::load(std::int8_t& t)
	{
		std::uint8_t u;
		load(u);
		t = (std::int8_t)u;
	}

	template<> inline void MessageReader::load(std::int16_t& t)
	{
		std::uint16_t u;
		load(u);
		t = (std::int16_t)u;
	}

	template<> inline void MessageReader::load(std::int32_t& t)
	{
		std::uint32_t u;
		load(u);
		t = (std::int32_t)u;
	}

	template<> inline void MessageReader::load(std::int64_t& t)
	{
		std::uint64_t u;
		load(u);
		t = (std::int64_t)u;
	}
}
