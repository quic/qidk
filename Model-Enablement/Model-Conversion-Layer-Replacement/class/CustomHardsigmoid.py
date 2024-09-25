#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

class CustomHardsigmoid(Module):
    __constants__ = ['inplace']
    inplace: bool
    def __init__(self, inplace : bool = False)->None:    
        super().__init__()
        self.inplace = inplace
    def forward(self, input: Tensor, inplace: bool = False) -> Tensor: 
        return torch.clamp((input*0.167+0.5), 0, 1)
