#include "mysqlutil.h"
#include <iostream>
using namespace std;

#define CATCH_SQL_ERR() catch (sql::SQLException& e) \
{\
	cout << "# ERR: SQLException in " << __FILE__;\
	cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;\
	cout << "# ERR: " << e.what();\
	cout << " (MySQL error code: " << e.getErrorCode();\
	cout << ", SQLState: " << e.getSQLState() << " )" << endl;\
\
	printf("Failed to connect to the Database! :( \n");\
}

MySQLUtil::MySQLUtil()
	: m_Connected(false)
	, m_Connection(nullptr)
	, m_Driver(nullptr)
	, m_ResultSet(nullptr)
	, m_Statement(nullptr)
{
}

MySQLUtil::~MySQLUtil()
{
	if (m_Connection != nullptr)
		delete m_Connection;

	if (m_ResultSet != nullptr)
		delete m_ResultSet;

	if (m_Statement != nullptr)
		delete m_Statement;
}

void MySQLUtil::ConnectToDatabase(const char* host, const char* username, const char* password, const char* schema)
{
	if (m_Connected) return;

	try
	{
		m_Driver = sql::mysql::get_mysql_driver_instance();
		m_Connection = m_Driver->connect(host, username, password);
		m_Statement = m_Connection->createStatement();

		// Set the Schema you want to use.
		m_Connection->setSchema(schema);
	}
	CATCH_SQL_ERR();

	printf("Successfully connected to the Database! :D \n");

	m_Connected = true;
}

void MySQLUtil::Disconnect()
{
	if (!m_Connected) return;

	m_Connection->close();

	m_Connected = false;
}


sql::PreparedStatement* MySQLUtil::PrepareStatement(const char* query)
{
	if (!m_Connected)
	{
		return nullptr;
	}
	return m_Connection->prepareStatement(query);
}

sql::ResultSet* MySQLUtil::Select(const char* query)
{
	// Add try catch blocks to each sql call
	m_ResultSet = m_Statement->executeQuery(query);
	return m_ResultSet;
}

int MySQLUtil::Update(const char* query)
{
	// Add try catch blocks to each sql call
	int count = m_Statement->executeUpdate(query);
	return count;
}

int MySQLUtil::Insert(const char* query)
{
	// Add try catch blocks to each sql call
	int count = m_Statement->executeUpdate(query);
	return count;
}

