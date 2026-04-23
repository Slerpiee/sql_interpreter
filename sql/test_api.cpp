#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "Table.hpp"

// Don't use 'using namespace table' to avoid ambiguity with struct Table
// Instead, use explicit qualification where needed

// Helper function to print test status
void printTestStatus(const std::string& testName, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

// Test 1: Create and open table
void testCreateAndOpenTable() {
    const std::string tableName = "test_table.dat";
    
    // Clean up if exists
    try {
        table::Table::remove(tableName);
    } catch (...) {}
    
    // Create table with schema
    std::vector<table::FieldDefinition> fields = {
        table::FieldDefinition("id", Long),
        table::FieldDefinition("name", Text, 50),
        table::FieldDefinition("age", Long)
    };
    
    table::Table::create(tableName, fields);
    printTestStatus("Create table", true);
    
    // Open table
    table::Table tbl(tableName);
    printTestStatus("Open table", true);
    
    // Verify field count
    assert(tbl.getFieldCount() == 3);
    printTestStatus("Field count is 3", true);
    
    // Verify field names
    assert(tbl.getFieldName(0) == "id");
    assert(tbl.getFieldName(1) == "name");
    assert(tbl.getFieldName(2) == "age");
    printTestStatus("Field names are correct", true);
    
    // Verify field types
    assert(tbl.getFieldType("id") == Long);
    assert(tbl.getFieldType("name") == Text);
    assert(tbl.getFieldType("age") == Long);
    printTestStatus("Field types are correct", true);
    
    // Verify field lengths
    assert(tbl.getFieldLength("name") == 50);
    printTestStatus("Field length for 'name' is 50", true);
    
    // Close table (automatic via destructor)
    printTestStatus("Close table (via destructor)", true);
}

// Test 2: Insert and read records
void testInsertAndReadRecords() {
    const std::string tableName = "test_insert.dat";
    
    // Clean up if exists
    try {
        table::Table::remove(tableName);
    } catch (...) {}
    
    // Create table
    std::vector<table::FieldDefinition> fields = {
        table::FieldDefinition("id", Long),
        table::FieldDefinition("value", Text, 30)
    };
    table::Table::create(tableName, fields);
    
    // Open and insert records
    {
        table::Table tbl(tableName);
        
        // Insert first record at beginning
        tbl.createNew();
        tbl.putLongNew("id", 1);
        tbl.putTextNew("value", "First");
        tbl.insertAtBeginning();
        printTestStatus("Insert record at beginning", true);
        
        // Insert second record at end
        tbl.createNew();
        tbl.putLongNew("id", 2);
        tbl.putTextNew("value", "Second");
        tbl.insertAtEnd();
        printTestStatus("Insert record at end", true);
        
        // Insert third record at current position
        tbl.moveFirst();
        tbl.createNew();
        tbl.putLongNew("id", 3);
        tbl.putTextNew("value", "Third");
        tbl.insertNew();
        printTestStatus("Insert record at current position", true);
    }
    
    // Read back records
    {
        table::Table tbl(tableName);
        
        // Move to first and verify
        tbl.moveFirst();
        assert(tbl.getLong("id") == 3);
        assert(tbl.getText("value") == "Third");
        printTestStatus("Read first record", true);
        
        // Move next and verify
        tbl.moveNext();
        assert(tbl.getLong("id") == 1);
        assert(tbl.getText("value") == "First");
        printTestStatus("Read second record", true);
        
        // Move next and verify
        tbl.moveNext();
        assert(tbl.getLong("id") == 2);
        assert(tbl.getText("value") == "Second");
        printTestStatus("Read third record", true);
        
        // Verify afterLast
        assert(!tbl.isAfterLast());
        tbl.moveNext();
        assert(tbl.isAfterLast());
        printTestStatus("afterLast detection", true);
    }
}

// Test 3: Edit records
void testEditRecords() {
    const std::string tableName = "test_edit.dat";
    
    // Clean up if exists
    try {
        table::Table::remove(tableName);
    } catch (...) {}
    
    // Create table
    std::vector<table::FieldDefinition> fields = {
        table::FieldDefinition("id", Long),
        table::FieldDefinition("data", Text, 50)
    };
    table::Table::create(tableName, fields);
    
    // Insert a record
    {
        table::Table tbl(tableName);
        tbl.createNew();
        tbl.putLongNew("id", 100);
        tbl.putTextNew("data", "Original");
        tbl.insertAtEnd();
    }
    
    // Edit the record
    {
        table::Table tbl(tableName);
        tbl.moveFirst();
        
        // Start edit
        tbl.startEdit();
        printTestStatus("Start edit", true);
        
        // Modify fields
        tbl.putLong("id", 200);
        tbl.putText("data", "Modified");
        printTestStatus("Put modified values", true);
        
        // Finish edit
        tbl.finishEdit();
        printTestStatus("Finish edit", true);
        
        // Verify changes
        tbl.moveFirst();
        assert(tbl.getLong("id") == 200);
        assert(tbl.getText("data") == "Modified");
        printTestStatus("Verify edited values", true);
    }
}

// Test 4: Delete records
void testDeleteRecords() {
    const std::string tableName = "test_delete.dat";
    
    // Clean up if exists
    try {
        table::Table::remove(tableName);
    } catch (...) {}
    
    // Create table
    std::vector<table::FieldDefinition> fields = {
        table::FieldDefinition("num", Long)
    };
    table::Table::create(tableName, fields);
    
    // Insert records
    {
        table::Table tbl(tableName);
        for (int i = 1; i <= 3; i++) {
            tbl.createNew();
            tbl.putLongNew("num", i * 10);
            tbl.insertAtEnd();
        }
    }
    
    // Delete middle record
    {
        table::Table tbl(tableName);
        tbl.moveFirst();  // num = 10
        tbl.moveNext();   // num = 20 (middle)
        tbl.deleteRecord();
        printTestStatus("Delete middle record", true);
    }
    
    // Verify remaining records
    {
        table::Table tbl(tableName);
        tbl.moveFirst();
        assert(tbl.getLong("num") == 10);
        tbl.moveNext();
        assert(tbl.getLong("num") == 30);
        tbl.moveNext();
        assert(tbl.isAfterLast());
        printTestStatus("Verify records after deletion", true);
    }
}

// Test 5: Navigation functions
void testNavigation() {
    const std::string tableName = "test_navigation.dat";
    
    // Clean up if exists
    try {
        table::Table::remove(tableName);
    } catch (...) {}
    
    // Create table
    std::vector<table::FieldDefinition> fields = {
        table::FieldDefinition("idx", Long)
    };
    table::Table::create(tableName, fields);
    
    // Insert 5 records
    {
        table::Table tbl(tableName);
        for (int i = 1; i <= 5; i++) {
            tbl.createNew();
            tbl.putLongNew("idx", i);
            tbl.insertAtEnd();
        }
    }
    
    // Test navigation
    {
        table::Table tbl(tableName);
        
        // Test beforeFirst on empty cursor
        tbl.moveFirst();
        assert(!tbl.isBeforeFirst());
        printTestStatus("Not beforeFirst after moveFirst", true);
        
        // Test moveLast
        tbl.moveLast();
        assert(tbl.getLong("idx") == 5);
        printTestStatus("moveLast works", true);
        
        // Test movePrevious
        tbl.movePrevious();
        assert(tbl.getLong("idx") == 4);
        printTestStatus("movePrevious works", true);
        
        // Test full iteration
        tbl.moveFirst();
        int expected = 1;
        while (!tbl.isAfterLast()) {
            assert(tbl.getLong("idx") == expected);
            expected++;
            tbl.moveNext();
        }
        assert(expected == 6);  // 5 records + 1
        printTestStatus("Full iteration works", true);
    }
}

// Test 6: Error handling
void testErrorHandling() {
    const std::string tableName = "test_errors.dat";
    
    // Try to open non-existent table
    bool exceptionThrown = false;
    try {
        table::Table tbl(tableName);
    } catch (const table::TableException& e) {
        exceptionThrown = true;
        std::cout << "  Caught expected exception: " << e.what() << std::endl;
    }
    printTestStatus("Exception on non-existent table", exceptionThrown);
    
    // Create valid table for further tests
    std::vector<table::FieldDefinition> fields = {
        table::FieldDefinition("val", Long)
    };
    table::Table::create(tableName, fields);
    
    // Try to get non-existent field
    {
        table::Table tbl(tableName);
        tbl.createNew();
        tbl.putLongNew("val", 42);
        tbl.insertAtEnd();
        
        bool fieldNotFound = false;
        try {
            tbl.getLong("nonexistent");
        } catch (const table::TableException& e) {
            fieldNotFound = true;
            std::cout << "  Caught expected exception: " << e.what() << std::endl;
        }
        printTestStatus("Exception on non-existent field", fieldNotFound);
    }
    
    // Clean up
    table::Table::remove(tableName);
}

int main() {
    std::cout << "=== Table API Tests ===" << std::endl << std::endl;
    
    try {
        std::cout << "--- Test 1: Create and Open Table ---" << std::endl;
        testCreateAndOpenTable();
        std::cout << std::endl;
        
        std::cout << "--- Test 2: Insert and Read Records ---" << std::endl;
        testInsertAndReadRecords();
        std::cout << std::endl;
        
        std::cout << "--- Test 3: Edit Records ---" << std::endl;
        testEditRecords();
        std::cout << std::endl;
        
        std::cout << "--- Test 4: Delete Records ---" << std::endl;
        testDeleteRecords();
        std::cout << std::endl;
        
        std::cout << "--- Test 5: Navigation Functions ---" << std::endl;
        testNavigation();
        std::cout << std::endl;
        
        std::cout << "--- Test 6: Error Handling ---" << std::endl;
        testErrorHandling();
        std::cout << std::endl;
        
        std::cout << "=== All Tests Completed Successfully! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
