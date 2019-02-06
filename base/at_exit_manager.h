#pragma once

#include "closure.h"

#include <vector>

class AtExitManager
{
public:
    AtExitManager() = default;
    virtual ~AtExitManager()
    {
        deal_with_callbacks();
    }

    void register_callback(std::shared_ptr<Closure> obj)
    {
        at_exit_callbacks_.push_back(obj);
    }

protected:
    //嗯嗯，处理回调是按照后进先出的顺序（栈）， 模拟函数返回时，栈上对象的析构顺序
    void deal_with_callbacks()
    {
        for (auto it = at_exit_callbacks_.rbegin(); it != at_exit_callbacks_.rend(); it++)
        {
            if (!(*it)->is_canceled())
            {
                (*it)->run();
            }
        }

        at_exit_callbacks_.clear();
    }

private:
    std::vector<std::shared_ptr<Closure>> at_exit_callbacks_;
};

