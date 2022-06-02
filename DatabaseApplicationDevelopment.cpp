#include <iostream>
#include <string>
#include <iomanip>
#include <occi.h>
using namespace oracle::occi;
using namespace std;

using oracle::occi::Environment;
using oracle::occi::Connection;

//create a struct Cart to store one kind of product
struct Cart
{
    int m_productId{};
    double m_price{};
    int m_quantity{};
    string m_name{};
};

//create a class User to store user's information
class User
{
    string userName = "dbs311_213b39";//userID
    string pass = "31955130";//password
    string oracle = "myoracle12c.senecacollege.ca:1521/oracle12c";
public:

    string getUserName() {
        return userName;
    }

    string getUserPass() {
        return pass;
    }

    string getOracle() {
        return oracle;
    }
};

// create a class Utility to store the help function such as the function to get an integer
class Utility
{
public:
    int getUnsignedInt(unsigned int min = -1, unsigned int max = -1) {
        bool flag = true;
        unsigned int ret = 0;
        while (flag) {
            string temp = "";
            getline(cin, temp);
            if (!temp.length())
                cout << "ERROR! You entered nothing! Input again: ";
            else {
                if (temp.find_first_not_of("0123456789") != string::npos)
                    cout << "ERROR! You entered a wrong type! Input again: ";
                else {
                    ret = stoi(temp);
                    if (min == max && max == -1)
                        flag = false;
                    else {
                        if (ret<min || ret>max)
                            cout << "You entered a wrong value. Enter an option (" << min << " - " << max << "): ";
                        else
                            flag = false;
                    }
                }
            }
        }
        return ret;
    }
};

//create a class Customer to store customer's information including customer's cart, which contain maximum 3 products
class Customer
{
    int m_customerId = 0;
    Cart m_cart[3];
    int m_cnt = 0;
    Connection* m_conn = nullptr;
    Utility u;
public:
    Customer() {}

    Customer(Connection* conn, int customerId) {
        m_conn = conn;
        m_customerId = customerId;
    }

    //create a function to help the customer to add different products into the cart
    int addToCart() {
        bool loop = true;
        while (loop && m_cnt < 3) {//while loop to ensure the number of product added to the cart is maximum 3
            cout << "Enter the product ID: ";
            int productId = u.getUnsignedInt();
            Cart cart;
            if (findProduct(m_conn, productId, cart)) {
                bool duplicate = false;
                for (int i = 0; !duplicate && i < m_cnt; i++) {//make sure the product will not be duplicate with those products added before
                    if (m_cart[i].m_productId == productId)
                        duplicate = true;
                }
                if (duplicate)
                    cout << "You have already added the product " << productId << " . Please try again..." << endl;
                else {
                    m_cart[m_cnt].m_price = cart.m_price;//store the information of product added
                    m_cart[m_cnt].m_productId = productId;
                    m_cart[m_cnt].m_name = cart.m_name;
                    int quantity = 0;
                    cout << "Product Price: " << m_cart[m_cnt].m_price << endl;
                    while (!quantity) {
                        cout << "Enter the product Quantity: ";
                        quantity = u.getUnsignedInt();
                        if (quantity > 0) {
                            m_cart[m_cnt].m_quantity = quantity;
                            m_cnt++;
                            if (m_cnt < 3) {
                                cout << "Enter 1 to add more products or 0 to checkout: ";
                                int choice = u.getUnsignedInt(0, 1);
                                if (!choice)
                                    loop = false;
                            }
                            else
                                loop = false;
                        }
                        else
                            cout << "ERROR! Enter an integer bigger than 0!" << endl;
                    }
                }
            }
            else
                cout << "The product does not exist. Please try again..." << endl;
        }
        return m_cnt;
    }

    //create the function to store the valid order and order lines' information into the oracle database.
    void checkout() {
        bool flag = true;
        while (flag) {
            cout << "Are you ready to checkout(Y/y) or Cancel (N/n): ";
            string input = "";
            getline(cin, input);
            if (input.length() != 1 && (input[0] != 'y' || input[0] != 'Y' || input[0] != 'n' || input[0] != 'N'))
                cout << "Wrong input. Please try again..." << endl;
            else if (input.length() == 1 && (input[0] == 'y' || input[0] == 'Y')) {
                Statement* statement = m_conn->createStatement();//the customer choose yes to store the order information
                statement->setSQL("BEGIN add_order(:1, :2); END;");//start the sql command
                statement->setInt(1, m_customerId);//set the customer's id to make sure the order belongs to this customer
                statement->registerOutParam(2, Type::OCCIINT);//get the new order id
                statement->executeUpdate();
                int orderId = statement->getInt(2);
                for (int i = 0; i < m_cnt; i++) {
                    statement->setSQL("BEGIN add_orderline (:1, :2, :3, :4, :5); END;");
                    statement->setInt(1, orderId);//set order id
                    statement->setInt(2, i + 1);//set item id
                    statement->setInt(3, m_cart[i].m_productId);//set product id
                    statement->setInt(4, m_cart[i].m_quantity);//set product quantity
                    statement->setDouble(5, m_cart[i].m_price);//set product price
                    statement->executeUpdate();
                }
                m_conn->terminateStatement(statement);
                cout << "The order is successfully completed." << endl << endl;
                flag = false;
            }
            else if (input.length() == 1 && (input[0] == 'n' || input[0] == 'N')) {
                cout << "The order is cancelled." << endl << endl;
                flag = false;
            }

        }
    }

    //create the function to check the product with product_id exists or not. If it exists, get the product price,name.
    //If it doesn't exist, the price will be zero.
    double findProduct(Connection* conn, int product_id, Cart& cart) {
        if (m_conn) {
            string command = string("BEGIN find_product(:1, :2, :3); END;");
            Statement* statement = m_conn->createStatement();
            statement->setSQL(command);
            statement->setInt(1, product_id);
            statement->registerOutParam(2, Type::OCCIDOUBLE);
            statement->registerOutParam(3, Type::OCCISTRING, 30);
            statement->executeUpdate();
            cart.m_price = statement->getDouble(2);
            cart.m_name = statement->getString(3);
            conn->terminateStatement(statement);
        }
        return cart.m_price;
    }

    //display all the products information stored in cart.
    void displayProducts() {
        int total = 0;
        cout << "--------------- Ordered Products XXXXXXXXX ---------------" << endl;
        for (int i = 0; i < m_cnt; i++) {
            cout << "---Item  " << i + 1 << "   Product ID: " << left << setw(8) << m_cart[i].m_productId
                << left << setw(15) << m_cart[i].m_name
                << "  Price: " << left << setw(5) << m_cart[i].m_price
                << "Quantity: " << m_cart[i].m_quantity << endl;
            total += m_cart[i].m_price * m_cart[i].m_quantity;
        }
        cout << "-----------------------------------------------------------" << endl;
        cout << "Total: " << total << endl;
        cout << "======================================================================" << endl;
    }
};

//create a class ManageOrder to manage the whole procedure
class ManageOrder
{
    Environment* m_env = nullptr;
    Customer* m_customer = nullptr;
    Connection* m_conn = nullptr;
    User m_user;
    Utility u;
public:
    ManageOrder() {}

    ~ManageOrder() {
        m_env->terminateConnection(m_conn);// When the program ends, terminate the connection with oracle.
        delete m_customer; //delete the dynamic allocation for customer.
    }

    //main menu for the procedure, 1 to login and 0 to exit
    int mainMenu() {
        bool flag = true;
        int choice = -1, min = 0, max = 1;
        while (flag) {
            cout << "*************Main Menu by XIE********************" << endl;
            cout << "1)      Login" << endl;
            cout << "0)      Exit" << endl;
            cout << "Enter an option(" << min << " - " << max << ") : ";
            choice = u.getUnsignedInt(0, 1);// to get the user' choice
            switch (choice) {
            case 1:
                createConn(m_user.getUserName(), m_user.getUserPass(), m_user.getOracle());//to login the oracle
                if (m_conn) {
                    cout << "\nEnter the customer ID: ";
                    int customerId = u.getUnsignedInt();//When the customer id is valid, show the menu to add products.
                    if (customerLogin(customerId)) {
                        m_customer = new Customer(m_conn, customerId);
                        cout << "\n-------------- Add Products to Cart XXXXXXXXX --------------" << endl;
                        if (m_customer->addToCart()) {
                            m_customer->displayProducts();
                            m_customer->checkout();
                        }
                    }
                    else {//if customer doesn't exist, prompt new choice
                        cout << "The customer " << customerId << " does not exist." << endl << endl;
                        delete m_customer;
                        m_customer = nullptr;
                    }
                }
                break;
            case 0://quit the menu and the program
                cout << "Thank you --- Good bye..." << endl;
                flag = false;
                break;
            }
        }
        return choice;
    }

    //to check to customerId is valid or not. If the id is valid, access to oracle and return to 1
    // If the id is not valid, then return to 0
    int customerLogin(int customerId) {
        int success = 0;
        if (m_conn) {
            string command = string("BEGIN find_customer(:1, :2); END;");
            Statement* statement = m_conn->createStatement();
            statement->setSQL(command);
            statement->setInt(1, customerId);
            statement->registerOutParam(2, Type::OCCIINT);
            statement->executeUpdate();
            success = statement->getInt(2);
            m_conn->terminateStatement(statement);
        }
        return success;
    }


    //create the connection with oracle by the user's information
    void createConn(const string user, const string password, const string oracle) {
        m_env = Environment::createEnvironment(Environment::DEFAULT);
        m_conn = m_env->createConnection(user, password, oracle);
    }
};

int main(void) {
    ManageOrder managerOder;
    try {// to catch the oracle's exception
        managerOder.mainMenu();
    }
    catch (SQLException& sqlException) {
        cout << sqlException.getErrorCode() << ": " << sqlException.getMessage();
    }
    return 0;
}


/*
SET SERVEROUTPUT ON;

CREATE OR REPLACE PROCEDURE find_customer(customer_id IN NUMBER, found OUT NUMBER)AS
name CUSTOMERS.CNAME%type;
BEGIN
  SELECT cname
  INTO name
  FROM customers
  WHERE cust_no = customer_id;
  found := 1;
EXCEPTION
WHEN no_data_found THEN
        found := 0;
END;


CREATE OR REPLACE PROCEDURE find_product (product_id IN NUMBER, price OUT products.prod_sell%TYPE, proName OUT products.prod_name%TYPE) AS

BEGIN
  SELECT prod_sell, prod_name
  INTO price, proName
  FROM products
  WHERE prod_no = product_id;
EXCEPTION
WHEN no_data_found THEN
        price := 0;
END;


CREATE OR REPLACE PROCEDURE add_order (customer_id IN NUMBER, new_order_id OUT NUMBER)AS
maxId orders.order_no%type;
BEGIN
  SELECT MAX(order_no)
  INTO maxId
  FROM orders;
  new_order_id := maxId + 1;
  INSERT INTO orders (order_no, cust_no, status, rep_no, order_dt)
  VALUES(new_order_id, customer_id, 'O', 174, sysdate);
END;


CREATE OR REPLACE PROCEDURE add_orderline (orderId IN orderlines.order_no%type,
                                itemId IN orderlines.line_no%type,
                                productId IN orderlines.prod_no%type,
                                quantity IN orderlines.qty%type,
                                price IN orderlines.price%type) AS

BEGIN
  INSERT INTO orderlines (order_no, line_no, prod_no, qty, price)
  VALUES(orderId, itemId, productId, quantity, price);
END;


SELECT DISTINCT o.cust_no,o.order_no, prod_no
        FROM orders o LEFT JOIN orderlines ol
                ON o.order_no = ol.order_no
        WHERE TO_DATE(o.order_dt,'DD-Mon-YYYY')>=TO_DATE('2021-01-01','YYYY-DD-MM');

*/