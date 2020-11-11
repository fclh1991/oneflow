"""
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
import oneflow as flow
import numpy as np
import oneflow.typing as tp
from test_util import GenArgList
import unittest
from collections import OrderedDict
from typing import List


def _compare_l1loss_with_np(
    input_shape, target_shape, device_type, machine_ids, device_counts
):
    input = np.random.random(size=input_shape).astype(np.float32)
    target = np.random.random(size=target_shape).astype(np.float32)

    assert device_type in ["cpu", "gpu"]

    flow.clear_default_session()
    flow.env.init()
    if device_type == "cpu":
        flow.config.cpu_device_num(device_counts)
    else:
        flow.config.gpu_device_num(device_counts)

    func_config = flow.FunctionConfig()

    @flow.global_function(type="train", function_config=func_config)
    def oneflow_l1loss(
        of_input: tp.Numpy.Placeholder(shape=input.shape),
        of_target: tp.Numpy.Placeholder(shape=target.shape),
    ) -> List[tp.Numpy]:
        x_var = flow.get_variable(
            shape=input.shape,
            dtype=flow.float32,
            initializer=flow.ones_initializer(),
            name="x_var",
        )

        with flow.scope.placement(device_type, machine_ids):
            l1loss = flow.nn.L1Loss(
                of_input, of_target, reduction="none", name="of_l1loss"
            )
            l1loss_mean = flow.nn.L1Loss(
                of_input, of_target, reduction="mean", name="of_l1loss_mean"
            )
            l1loss_sum = flow.nn.L1Loss(
                of_input, of_target, reduction="sum", name="of_l1loss_sum"
            )

            out = l1loss + x_var
            flow.optimizer.SGD(
                flow.optimizer.PiecewiseConstantScheduler([], [1e-3]), momentum=0
            ).minimize(out)

        return [l1loss, l1loss_mean, l1loss_sum]

    def np_l1loss(np_input, np_target):
        np_l1 = np.abs(np_target - np_input)
        np_l1_mean = np.mean(np_l1)
        np_l1_sum = np.sum(np_l1)

        return [np_l1, np_l1_mean, np_l1_sum]

    of_out_l1loss = oneflow_l1loss(input, target)
    np_out_l1loss = np_l1loss(input, target)

    assert np.allclose(of_out_l1loss[0], np_out_l1loss[0])

    for i in range(1, len(np_out_l1loss)):
        # Numpy return a scalar(no shape), but oneflow return a N-D tensor.
        # I need to get it by using index [0]
        assert np.allclose(of_out_l1loss[i][0], np_out_l1loss[i])


@flow.unittest.skip_unless_1n1d()
class Testl1loss1n1d(flow.unittest.TestCase):
    def test_l1loss_cpu(test_case):
        arg_dict = OrderedDict()
        arg_dict["input_shape"] = [(3, 16, 16)]
        arg_dict["target_shape"] = [(3, 16, 16)]
        arg_dict["device_type"] = ["cpu"]
        arg_dict["machine_ids"] = ["0:0"]
        arg_dict["device_counts"] = [1]
        for arg in GenArgList(arg_dict):
            _compare_l1loss_with_np(*arg)

    def test_l1loss_gpu(test_case):
        arg_dict = OrderedDict()
        arg_dict["input_shape"] = [(3, 16, 32)]
        arg_dict["target_shape"] = [(3, 16, 32)]
        arg_dict["device_type"] = ["gpu"]
        arg_dict["machine_ids"] = ["0:0"]
        arg_dict["device_counts"] = [1]
        for arg in GenArgList(arg_dict):
            _compare_l1loss_with_np(*arg)


@flow.unittest.skip_unless_1n2d()
class Testrange1n2d(flow.unittest.TestCase):
    def test_l1loss_gpu_1n2d(test_case):
        arg_dict = OrderedDict()
        arg_dict["input_shape"] = [(3, 32, 16)]
        arg_dict["target_shape"] = [(3, 32, 16)]
        arg_dict["device_type"] = ["gpu"]
        arg_dict["machine_ids"] = ["0:0-1"]
        arg_dict["device_counts"] = [2]
        for arg in GenArgList(arg_dict):
            _compare_l1loss_with_np(*arg)


if __name__ == "__main__":
    unittest.main()