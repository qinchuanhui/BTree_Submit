//
// Created by 93738 on 2019/6/3.
//
class Integer {
private:
    int data;
public:
    Integer(const int &value) : data(value) {}
    Integer(const Integer &other) : data(other.data) {}
    bool operator==(const Integer &t)
    {
        return data == t.data;
    }
};